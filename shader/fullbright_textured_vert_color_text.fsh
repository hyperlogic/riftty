uniform vec4 color;
uniform sampler2D tex;
uniform float lod_bias;

varying vec2 frag_uv;
varying vec4 frag_color;

void main(void)
{
    // if tex is an alpha only texture, the color will be zero.
    // so lets just use the color directly
    gl_FragColor.rgb = frag_color.rgb;
    gl_FragColor.a = frag_color.a * texture2D(tex, frag_uv, lod_bias).a;
}

