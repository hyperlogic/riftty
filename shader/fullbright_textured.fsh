uniform vec4 color;
uniform sampler2D tex;

varying vec2 frag_uv;

void main(void)
{
    gl_FragColor = color * texture2D(tex, frag_uv);
}
