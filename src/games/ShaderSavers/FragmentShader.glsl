#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// --- hash / noise helpers ---
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
           (c - a) * u.y * (1.0 - u.x) +
           (d - b) * u.x * u.y;
}

// smooth vector field (key to "flurry" motion)
vec2 field(vec2 p) {
    float n1 = noise(p * 1.5 + time * 0.15);
    float n2 = noise(p * 1.5 - time * 0.12 + 10.0);

    float a = n1 * 6.28318;
    float b = n2 * 6.28318;

    return vec2(cos(a), sin(b));
}

// iterative flow advection (cheap "particle tracing")
vec3 flurry(vec2 uv) {
    vec2 p = uv;
    vec3 col = vec3(0.0);

    float t = time * 0.35;

    for (int i = 0; i < 18; i++) {
        vec2 f = field(p + float(i) * 0.03);

        // curl-ish motion
        p += f * 0.015;

        float d = length(uv - p);

        // glowing streaks
        float intensity = 0.015 / (d + 0.02);

        vec3 c = 0.5 + 0.5 * cos(vec3(0.0, 2.0, 4.0) + t + float(i) * 0.3);

        col += c * intensity;
    }

    return col;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    vec3 col = flurry(uv);

    // slight vignette like macOS screensaver
    float vignette = smoothstep(1.2, 0.2, length(uv));
    col *= vignette;

    // tone mapping
    col = 1.0 - exp(-col * 1.8);

    FragColor = vec4(col, 1.0);
}