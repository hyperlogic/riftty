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
    to_s.split('_').map() {|s| s[0,1].upcase + s[1..-1]}.join
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

$attrib_size = {
  :float => 1,
  :vec2 => 2,
  :vec3 => 3,
  :vec4 => 4,
}

$cpp_header = <<CODE
#ifndef <%= prog.name.upcase %>_H
#define <%= prog.name.upcase %>_H

#include "shader.h"
#include "abaci.h"
#include <vector>

class <%= prog.name.camelcase %> : public Shader
{
public:
    enum Locs {
<% prog.uniforms.each do |u| %>
        <%= u.name.camelcase %>UniformLoc,
<% end %>
<% prog.attribs.each do |a| %>
        <%= a.name.camelcase %>AttribLoc,
<% end %>
        NumLocs
    };

    <%= prog.name.camelcase %>() : Shader()
    {
        for (int i = 0; i < NumLocs; i++)
            m_locs[i] = -1;
    }

    void apply(const Shader* prevShader, const float* attribPtr) const
    {
<% prog.uniforms.each do |u| %>
        if (m_locs[<%= u.name.camelcase %>UniformLoc] < 0)
            m_locs[<%= u.name.camelcase %>UniformLoc] = getUniformLoc("<%= u.name %><%= u.class == BasicType ? "" : "[0]" %>");
        assert(m_locs[<%= u.name.camelcase %>UniformLoc] >= 0);
<% end %>
<% prog.attribs.each do |a| %>
        if (m_locs[<%= a.name.camelcase %>AttribLoc])
            m_locs[<%= a.name.camelcase %>AttribLoc] = getAttribLoc("<%= a.name %>");
        assert(m_locs[<%= a.name.camelcase %>AttribLoc] >= 0);
<% end %>

        Shader::apply(prevShader);

<% prog.uniforms.each do |u| %>
        <%= prog.gen_uniform_call(u) %>
<% end %>
<% stride = prog.attribs.map {|a| 4 * prog.calc_attrib_size(a)}.reduce {|a, b| a + b} %>
<% offset = 0 %>
<% prog.attribs.each do |a| %>
        <%= prog.gen_attrib_call(a, offset, stride) %>
<%     offset += prog.calc_attrib_size(a) %>
<% end %>
    }

    // uniform accessors
<% prog.uniforms.each do |u| %>
    <%= prog.uniform_ref_type(u) %> get<%= u.name.camelcase %>() const { return m_<%= u.name.camelcase.uncapitalize %>; }
    void set<%= u.name.camelcase %>(<%= prog.uniform_ref_type(u) %> v) { m_<%= u.name.camelcase.uncapitalize %> = v; }
<% end %>

protected:
    mutable int m_locs[NumLocs];
<% prog.uniforms.each do |u| %>
    <%= prog.uniform_decl_type(u) %> m_<%= u.name.camelcase.uncapitalize %>;
<% end %>

};

#endif // #define <%= prog.name.upcase %>_H
CODE

BasicType = Struct.new(:type, :name)
ArrayType = Struct.new(:type, :name, :count)

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
    assert {uniform.class == BasicType}
    textures = uniforms.find_all {|u| u.type == :sampler2D}
    textures.find_index {|u| u.name == uniform.name}
  end

  def gen_uniform_call(u)
    loc = "m_locs[#{u.name.camelcase}UniformLoc]"
    count = u.class == BasicType ? 1 : u.count
    value = "m_#{u.name.camelcase.uncapitalize}"
    assert {$uniform_call[u.type]}

    case u.type
    when :int
      if u.class == BasicType
        "#{$uniform_call[u.type]}(#{loc}, #{count}, (int*)&#{value});"
      else
        "#{$uniform_call[u.type]}(#{loc}, #{count}, (int*)&#{value}[0]);"
      end
    when :sampler2D
      stage = uniform_to_tex_stage(u)
      assert {stage && u.class == BasicType}  # ArrayType not supported on samplers!
      "glUniform1i(#{loc}, #{stage}); glActiveTexture(GL_TEXTURE0 + #{stage}); glBindTexture(GL_TEXTURE_2D, #{value});"
    when :mat4
      if u.class == BasicType
        "#{$uniform_call[u.type]}(#{loc}, #{count}, false, (float*)&#{value});"
      else
        "#{$uniform_call[u.type]}(#{loc}, #{count}, false, (float*)&#{value}[0]);"
      end
    else
      if u.class == BasicType
        "#{$uniform_call[u.type]}(#{loc}, #{count}, (float*)&#{value});"
      else
        "#{$uniform_call[u.type]}(#{loc}, #{count}, (float*)&#{value}[0]);"
      end
    end
  end

  def gen_attrib_call(a, offset, stride)
    assert {a.class == BasicType}  # array types not supported on attribs
    assert {a && offset && stride}

    size = calc_attrib_size(a)
    assert {size}

    loc = "m_locs[#{a.name.camelcase}AttribLoc]"
    "glVertexAttribPointer(#{loc}, #{size}, GL_FLOAT, false, #{stride}, attribPtr + #{offset});"
  end

  def calc_attrib_size(a)
    assert {a.class == BasicType}  # array types not supported on attribs
    assert {a}
    size = $attrib_size[a.type]
    assert {size}
    size
  end

  def uniform_decl_type(u)
    if u.class == BasicType
      $cpp_decl_type[u.type]
    else
      "std::vector<#{$cpp_decl_type[u.type]}>"
    end
  end

  def uniform_ref_type(u)
    if u.class == BasicType
      $cpp_ref_type[u.type]
    else
      "const std::vector<#{$cpp_decl_type[u.type]}>&"
    end
  end
end

progs = [Prog.new("fullbright_shader",
                  "shader/fullbright.vsh",
                  "shader/fullbright.fsh",
                  [BasicType.new(:vec4, :color),
                   BasicType.new(:mat4, :mat)],
                  [BasicType.new(:vec3, :pos)]),
         Prog.new("fullbright_textured_shader",
                  "shader/fullbright_textured.vsh",
                  "shader/fullbright_textured_text.fsh",
                  [BasicType.new(:vec4, :color),
                   BasicType.new(:mat4, :mat),
                   BasicType.new(:sampler2D, :tex),
                   BasicType.new(:float, :lod_bias)],
                  [BasicType.new(:vec3, :pos),
                   BasicType.new(:vec2, :uv)]),
         Prog.new("phong_textured_shader",
                  "shader/phong_textured.vsh",
                  "shader/phong_textured.fsh",
                  [BasicType.new(:vec4, :color),
                   BasicType.new(:mat4, :full_mat),
                   BasicType.new(:mat4, :world_mat),
                   BasicType.new(:mat4, :world_normal_mat),
                   BasicType.new(:sampler2D, :tex),
                   BasicType.new(:int, :num_lights),
                   ArrayType.new(:vec3, :light_world_pos, 8),
                   ArrayType.new(:vec3, :light_color, 8),
                   ArrayType.new(:float, :light_strength, 8)],
                  [BasicType.new(:vec3, :pos),
                   BasicType.new(:vec2, :uv),
                   BasicType.new(:vec3, :normal)])
        ]

progs.each do |prog|
  prog.build
end
