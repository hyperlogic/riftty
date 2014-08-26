
//
// No lighting at all, vertex color, single texture
//

uniform mat4 mat;
attribute vec3 pos;
attribute vec2 uv;
attribute vec4 color;

varying vec2 frag_uv;
varying vec4 frag_color;

void main(void)
{
    // xform pos into screen space
    gl_Position = mat * vec4(pos, 1);
    frag_uv = uv;
    frag_color = color;
}
