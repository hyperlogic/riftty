
//
// No lighting at all, solid color, single texture
//

uniform vec4 color;
uniform mat4 mat;
attribute vec3 pos;
attribute vec2 uv;

varying vec2 frag_uv;

void main(void)
{
    // xform pos into screen space
    gl_Position = mat * vec4(pos, 1);
    frag_uv = uv;
}
