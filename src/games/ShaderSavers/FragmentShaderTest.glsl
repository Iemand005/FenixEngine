#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// --- cheap hash ---
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// --- smooth pseudo flow field ---
vec2 flow(vec2 p, float t) {
    float n = hash(floor(p * 2.0));

    float a = n * 6.28318 + sin(t + n * 6.0);
    return vec2(cos(a), sin(a));
}

// sample “trail” by integrating backwards in time
vec3 sampleTrail(vec2 uv, float t) {
    vec2 p = uv * 2.0 - 1.0;

    vec3 col = vec3(0.0);

    // we simulate persistence by sampling past motion
    for (int i = 0; i < 40; i++) {

        float ti = t - float(i) * 0.05;

        vec2 f = flow(p, ti);

        // integrate backwards to create streaks
        p += f * 0.02;

        float d = length(uv - (p * 0.5 + 0.5));

        // sharp “strand core”
        float strand = 0.01 / (d + 0.002);

        // color cycling like Flurry
        vec3 c = 0.6 + 0.4 * cos(vec3(0.0, 2.0, 4.0) + ti * 0.7);

        col += c * strand;
    }

    return col;
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;

    vec3 col = sampleTrail(uv, time);

    // subtle fade curve (important for Flurry feel)
    col = 1.0 - exp(-col * 1.4);

    // soft vignette like macOS fullscreen glow
    float vignette = smoothstep(1.2, 0.2, length(uv - 0.5));
    col *= vignette;

    FragColor = vec4(col, 1.0);
}