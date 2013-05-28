uniform vec4 color;
uniform sampler2D tex;

varying vec2 frag_uv;

void main(void)
{
    // if tex is an alpha only texture, the color will be zero.
    // so lets just use the color directly
    gl_FragColor.rgb = color.rgb;
    gl_FragColor.a = color.a * texture2D(tex, frag_uv).a;
}
