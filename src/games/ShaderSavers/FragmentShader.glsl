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

// distance from point p to segment a-b
float lineDist(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;

    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 center = vec2(0.5);

    vec3 col = vec3(0.0);

    float t = time * 0.6;

    for (int i = 0; i < 140; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));
        float seed = hash(fi * 78.233);

        float speed = 0.4 + hash(fi * 91.7) * 2.0;

        // current and previous motion positions
        float p1 = fract(seed + t * speed);
        float p0 = fract(seed + (t - 0.03) * speed);

        // acceleration outward (warp feel)
        float d1 = p1 * p1;
        float d0 = p0 * p0;

        vec2 pos1 = center + dir * d1 * 0.8;
        vec2 pos0 = center + dir * d0 * 0.8;

        vec2 p = uv;

        float dist = lineDist(p, pos0, pos1);

        // thin line
        float line = step(dist, 0.0025);

        float brightness = 0.6 + hash(fi * 9.1) * 0.7;

        col += vec3(line * brightness);
    }

    FragColor = vec4(col, 1.0);
}