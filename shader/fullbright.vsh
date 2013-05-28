
//
// No lighting at all, solid color
//

uniform vec4 color;
uniform mat4 mat;
attribute vec3 pos;

void main(void)
{
    gl_Position = mat * vec4(pos, 1);
}
