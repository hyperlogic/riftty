uniform vec4 color;
uniform sampler2D tex;

varying vec2 frag_uv;
varying vec4 frag_color;

void main(void)
{
    gl_FragColor = frag_color * texture2D(tex, frag_uv);
}
