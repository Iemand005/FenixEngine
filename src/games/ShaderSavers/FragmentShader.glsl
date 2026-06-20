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
    float t = time * 0.6;

    for (int i = 0; i < 160; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));

        float seed = hash(fi * 78.233);

        float speed = mix(40.0, 140.0, hash(fi * 91.7));

        // smooth looping WITHOUT hard fract jump artifacts
        float life = seed + t * speed / 500.0;

        float p1 = fract(life);
        float p0 = fract(life - 0.01);

        // fade near reset to remove "pop lines"
        float fade1 = smoothstep(0.0, 0.1, p1) * smoothstep(1.0, 0.9, p1);
        float fade0 = smoothstep(0.0, 0.1, p0) * smoothstep(1.0, 0.9, p0);

        float d1 = p1 * p1;
        float d0 = p0 * p0;

        vec2 pos1 = center + dir * d1 * (resolution.y * 0.6);
        vec2 pos0 = center + dir * d0 * (resolution.y * 0.6);

        float d = lineDist(frag, pos0, pos1);

        float line = step(d, 0.5);

        float brightness = 0.7 + hash(fi * 9.1) * 0.5;

        col += vec3(line * brightness * fade1 * fade0);
    }

    FragColor = vec4(col, 1.0);
}