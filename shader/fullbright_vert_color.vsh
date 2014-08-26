
//
// No lighting at all, solid color
//

uniform mat4 mat;
attribute vec3 pos;
attribute vec4 color;

varying vec4 frag_color;

void main(void)
{
    gl_Position = mat * vec4(pos, 1);
    frag_color = color;
}
