#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;
uniform sampler2D prevFrame;

vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

vec2 flow(vec2 p) {
    float t = time * 0.15;

    vec2 n = hash2(floor(p * 2.0));

    float a = n.x * 6.28318 + sin(t + n.y * 6.0);
    return vec2(cos(a), sin(a));
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;

    // --- feedback decay (this is Flurry's "memory") ---
    vec3 prev = texture(prevFrame, uv).rgb;
    prev *= 0.985;

    // --- advect UV through flow field ---
    vec2 p = uv * 2.0 - 1.0;

    vec3 col = vec3(0.0);

    // layered "strand" accumulation
    for (int i = 0; i < 6; i++) {
        vec2 f = flow(p * (1.2 + float(i) * 0.15));

        p += f * 0.03;

        float d = length(gl_FragCoord.xy / resolution - (p * 0.5 + 0.5));

        float glow = 0.02 / (d + 0.01);

        vec3 c = 0.5 + 0.5 * cos(vec3(0.0, 2.0, 4.0) + time * 0.6 + float(i));

        col += c * glow;
    }

    // combine with history (this is what makes it "Flurry-like")
    col += prev;

    // slight tone shaping
    col = 1.0 - exp(-col * 1.2);

    FragColor = vec4(col, 1.0);
}