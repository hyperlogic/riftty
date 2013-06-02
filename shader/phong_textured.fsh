uniform vec4 color;
//uniform sampler2D tex;

#define MAX_LIGHTS 8
uniform vec3 light_world_pos[MAX_LIGHTS];
uniform vec3 light_color[MAX_LIGHTS];
uniform float light_strength[MAX_LIGHTS];
uniform int num_lights;

varying vec2 frag_uv;
varying vec3 frag_world_pos;
varying vec3 frag_world_normal;

void main(void)
{
    vec3 d;
    vec3 accum = vec3(0, 0, 0);
    float len, att, len_over_str, intensity;
    int i;
    for (i = 0; i < num_lights; i++) {
        d = light_world_pos[i] - frag_world_pos;
        len = length(d);
        len_over_str = len / light_strength[i];
        att = min(1.0 / (len_over_str * len_over_str), 1.0);
        intensity = max(dot(normalize(d), frag_world_normal), 0.0) * att;
        accum += light_color[i] * intensity;
    }

    gl_FragColor = vec4(accum, 1) * color; //* texture2D(tex, frag_uv);
}
