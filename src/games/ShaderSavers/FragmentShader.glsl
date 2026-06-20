#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec2 hash2(float n) {
    return vec2(hash(n), hash(n + 17.0)) * 2.0 - 1.0;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    vec3 col = vec3(0.0);
    float t = time * 0.6;

    for (int i = 0; i < 140; i++)
    {
        float fi = float(i);

        // random direction from center
        vec2 dir = normalize(hash2(fi * 12.989));

        // unique phase per star
        float seed = hash(fi * 78.233);

        // normalized lifetime (0..1 looping)
        float p = fract(seed + t * (0.4 + hash(fi * 91.7) * 2.0));

        // IMPORTANT: acceleration outward (slow start -> fast edge)
        float accel = p * p;

        // outward distance
        float dist = accel * 2.5;

        vec2 pos = dir * dist;

        // sharp pixel star size (constant small square-ish point)
        float size = 0.008;

        vec2 diff = uv - pos;

        // HARD star (no blur)
        float star =
            step(abs(diff.x), size) *
            step(abs(diff.y), size);

        // optional slight brightness variation
        float brightness = 0.8 + hash(fi * 9.1) * 0.4;

        col += vec3(star * brightness);
    }

    FragColor = vec4(col, 1.0);
}