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

// distance from point p to segment a-b (in pixel space)
float lineDist(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;

    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main()
{
    vec2 frag = gl_FragCoord.xy;

    vec2 center = resolution * 0.5;
    vec3 col = vec3(0.0);

    float t = time * 0.7;

    for (int i = 0; i < 160; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));
        float seed = hash(fi * 78.233);

        // controlled speed range (prevents crazy fast stars)
        float speed = mix(30.0, 180.0, hash(fi * 91.7));

        // normalized life cycle
        float p1 = fract(seed + t * speed / 400.0);
        float p0 = fract(seed + (t - 0.01) * speed / 400.0);

        // acceleration outward
        float d1 = p1 * p1;
        float d0 = p0 * p0;

        // convert to pixel positions
        vec2 pos1 = center + dir * d1 * (resolution.y * 0.6);
        vec2 pos0 = center + dir * d0 * (resolution.y * 0.6);

        // pixel-space line distance
        float d = lineDist(frag, pos0, pos1);

        // TRUE 1-pixel line (no thickness)
        float line = step(d, 0.5);

        float brightness = 0.7 + hash(fi * 9.1) * 0.5;

        col += vec3(line * brightness);
    }

    FragColor = vec4(col, 1.0);
}