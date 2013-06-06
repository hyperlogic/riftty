uniform mat4 View;
uniform mat4 Texm;
attribute vec4 Position;
attribute vec2 TexCoord;
varying  vec2 oTexCoord;
void main()
{
   gl_Position = View * Position;
   oTexCoord = vec2(Texm * vec4(TexCoord,0,1));
   oTexCoord.y = 1.0-oTexCoord.y;
}
