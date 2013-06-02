uniform vec4 color;
uniform mat4 full_mat;
uniform mat4 world_mat;
uniform mat4 world_normal_mat;
//uniform sampler2D tex;

#define MAX_LIGHTS 8
uniform vec3 light_world_pos[MAX_LIGHTS];
uniform vec3 light_color[MAX_LIGHTS];
uniform float light_strength[MAX_LIGHTS];
uniform int num_lights;

attribute vec3 pos;
attribute vec2 uv;
attribute vec3 normal;

varying vec2 frag_uv;
varying vec3 frag_world_pos;
varying vec3 frag_world_normal;

void main(void)
{
    gl_Position = full_mat * vec4(pos, 1);
    frag_uv = uv;
    frag_world_pos = vec3(world_mat * vec4(pos, 1));
    frag_world_normal = vec3(world_normal_mat * vec4(normal, 1));
}
