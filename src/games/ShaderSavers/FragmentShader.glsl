#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// simple hash for pseudo-randomness
float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec2 hash2(float n) {
    return vec2(hash(n), hash(n + 1.0));
}

void main()
{
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    vec3 col = vec3(0.0);

    float t = time * 0.8;

    // number of stars (fake instancing)
    for (int i = 0; i < 120; i++)
    {
        float fi = float(i);

        // random direction from center
        vec2 dir = normalize(hash2(fi * 13.13) * 2.0 - 1.0);

        // random start offset so they don't sync
        float seed = hash(fi * 91.7);

        // radial distance (wraps over time)
        float speed = mix(0.4, 3.5, hash(fi * 7.1));
        float dist = fract(seed + t * speed);

        // make stars move outward from center
        vec2 pos = dir * dist * 2.5;

        // perspective feel: faster when farther out
        float depth = dist;
        float size = mix(0.012, 0.0015, depth);

        float d = length(uv - pos);

        // sharp star core
        float star = smoothstep(size, 0.0, d);

        // brightness variation
        float brightness = mix(0.6, 1.5, hash(fi * 33.3)) * (0.3 + depth);

        col += vec3(star * brightness);
    }

    // slight glow / tone shaping
    col = 1.0 - exp(-col);

    FragColor = vec4(col, 1.0);
}