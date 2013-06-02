#!/usr/bin/ruby

# I hate writting OpenGL shader glue code in C++, so I wrote this.
# the input is a set of uniforms and attribs
# the output is a c++ class that will:
#   1) provide a nice interface to get and set these values.
#   2) cache the program indices for each uniform and attrib.
#   3) bind the program, uniform and attribs for rendering.

require('erb')

def assert &block
  raise AssersionFailure unless yield
end

class Object
  def camelcase
    to_s.split('_').map(&:capitalize).join
  end
end

class String
  def uncapitalize
    self[0, 1].downcase + self[1..-1]
  end
end

$cpp_ref_type = {
  :float => 'float',
  :int => 'int',
  :vec2 => 'const Vector2f&',
  :vec3 => 'const Vector3f&',
  :vec4 => 'const Vector4f&',
  :mat4 => 'const Matrixf&',
  :sampler2D => 'int'
}

$cpp_decl_type = {
  :float => 'float',
  :int => 'int',
  :vec2 => 'Vector2f',
  :vec3 => 'Vector3f',
  :vec4 => 'Vector4f',
  :mat4 => 'Matrixf',
  :sampler2D => 'int'
}

$uniform_call = {
  :float => 'glUniform1fv',
  :int => 'glUniform1iv',
  :vec2 => 'glUniform2fv',
  :vec3 => 'glUniform3fv',
  :vec4 => 'glUniform4fv',
  :mat4 => 'glUniformMatrix4fv',
  :sampler2D => 'glUniform1iv',
}

def gen_uniform_call(u)

  loc = "m_locs[#{u.first[0].camelcase}UniformLoc]"
  count = 1
  value = "m_#{u.first[0].camelcase.uncapitalize}"
  type = u.first[1]

  assert {$uniform_call[type]}

  case type
  when :int
    "#{$uniform_call[type]}(#{loc}, #{count}, (int*)&#{value});"
  when :sampler2D
    # TODO: calculate stage by using Prog#uniform_to_tex_stage
    stage = 0
    "glUniform1i(#{loc}, #{stage}); glActiveTexture(GL_TEXTURE0 + #{stage}); glBindTexture(GL_TEXTURE_2D, #{value});"
  when :mat4
    "#{$uniform_call[type]}(#{loc}, #{count}, false, (float*)&#{value});"
  else
    "#{$uniform_call[type]}(#{loc}, #{count}, (float*)&#{value});"
  end
end

$attrib_size = {
  :float => 1,
  :vec2 => 2,
  :vec3 => 3,
  :vec4 => 4,
}

def calc_attrib_size(a)
  assert {a}
  type = a.first[1]
  assert {type}
  size = $attrib_size[type]
  assert {size}
  size
end

def gen_attrib_call(a, offset, stride)
  assert {a && offset && stride}

  size = calc_attrib_size(a)
  assert {size}

  loc = "m_locs[#{a.first[0].camelcase}AttribLoc]"
  "glVertexAttribPointer(#{loc}, #{size}, GL_FLOAT, false, #{stride}, attribPtr + #{offset});"
end
$cpp_header = <<CODE
#ifndef <%= prog.name.upcase %>_H
#define <%= prog.name.upcase %>_H

#include "shader.h"
#include "abaci.h"

class <%= prog.name.camelcase %> : public Shader
{
public:
    enum Locs {
<% prog.uniforms.each do |u| %>
        <%= u.first[0].camelcase %>UniformLoc,
<% end %>
<% prog.attribs.each do |a| %>
        <%= a.first[0].camelcase %>AttribLoc,
<% end %>
        NumLocs
    };

    <%= name.camelcase %>() : Shader()
    {
        for (int i = 0; i < NumLocs; i++)
            m_locs[i] = -1;
    }

    void apply(const Shader* prevShader, const float* attribPtr) const
    {
<% prog.uniforms.each do |u| %>
        if (m_locs[<%= u.first[0].camelcase %>UniformLoc])
            m_locs[<%= u.first[0].camelcase %>UniformLoc] = getUniformLoc("<%= u.first[0] %>");
        assert(m_locs[<%= u.first[0].camelcase %>UniformLoc] >= 0);
<% end %>
<% prog.attribs.each do |a| %>
        if (m_locs[<%= a.first[0].camelcase %>AttribLoc])
            m_locs[<%= a.first[0].camelcase %>AttribLoc] = getAttribLoc("<%= a.first[0] %>");
        assert(m_locs[<%= a.first[0].camelcase %>AttribLoc] >= 0);
<% end %>

        Shader::apply(prevShader);

<% prog.uniforms.each do |u| %>
        <%= gen_uniform_call(u) %>
<% end %>
<% stride = prog.attribs.map {|a| 4 * calc_attrib_size(a)}.reduce {|a, b| a + b} %>
<% offset = 0 %>
<% prog.attribs.each do |a| %>
        <%= gen_attrib_call(a, offset, stride) %>
<%     offset += calc_attrib_size(a) %>
<% end %>
    }

    // uniform accessors
<% prog.uniforms.each do |u| %>
    <%= $cpp_ref_type[u.first[1]] %> get<%= u.first[0].camelcase %>() const { return m_<%= u.first[0].camelcase.uncapitalize %>; }
    void set<%= u.first[0].camelcase %>(<%= $cpp_ref_type[u.first[1]] %> v) { m_<%= u.first[0].camelcase.uncapitalize %> = v; }
<% end %>

protected:
    mutable int m_locs[NumLocs];
<% prog.uniforms.each do |u| %>
    <%= $cpp_decl_type[u.first[1]] %> m_<%= u.first[0].camelcase.uncapitalize %>;
<% end %>

};

#endif // #define <%= prog.name.upcase %>_H
CODE

class Prog < Struct.new(:name, :vsh, :fsh, :uniforms, :attribs)
  def build
    filename = "#{name.camelcase}.h"

    # writable
    File.chmod(0666, filename) if File.exists?(filename)

    File.open(filename, "w") do |f|
      prog = self
      erb = ERB.new($cpp_header, 0, "<>")
      f.write erb.result(binding)
    end

    # readonly
    File.chmod(0444, filename)
  end

  def uniform_to_tex_stage uniform
    textures = uniforms.find_all {|u| u.first[1] == :sampler2D}
    textures.find_index {|u| u.first[0] == uniform.first}
  end
end

LightStruct = {:world_pos => :vec3, :color => :vec3, :strength => :float}

progs = [Prog.new("fullbright_shader",
                  "shader/fullbright.vsh", "shader/fullbright.fsh",
                  [{:color => :vec4}, {:mat => :mat4}],
                  [{:pos => :vec3}]),
         Prog.new("fullbright_textured_shader",
                  "shader/fullbright_textured.vsh", "shader/fullbright_textured_text.fsh",
                  [{:color => :vec4}, {:mat => :mat4}, {:tex => :sampler2D}],
                  [{:pos => :vec3}, {:uv => :vec2}]),
         Prog.new("phong_textured_shader",
                  "shader/phong_textured.vsh", "shader/phong_textured.fsh",
                  [{:color => :vec4}, {:full_mat => :mat4}, {:world_mat => :mat4},
                   {:world_normal_mat => :mat4}, {:tex => :sampler2D}, {:num_lights => :int},
                   {:lights => Array.new(8, LightStruct)}],
                  [{:pos => :vec3}, {:uv => :vec2}, {:normal => :vec3}])
        ]

progs[0, 2].each do |prog|
  prog.build
end

