#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

vec2 hash2(float n)
{
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
    vec2 uv = frag / resolution;

    vec3 col = vec3(0.0);

    float t = time * 2.0;

    for (int i = 0; i < 200; i++)
    {
        float fi = float(i);

        // FULL SCREEN PARTICLES (no center bias)
        vec2 p = hash2(fi * 12.989);

        vec2 pos0 = p;
        vec2 pos1 = p;

        float speed = mix(0.2, 1.5, hash(fi * 91.7));

        // motion in screen space (NO radial system)
        vec2 dir = normalize(hash2(fi * 78.233));

        pos1 += dir * fract(t * speed);
        pos0 += dir * fract((t - 0.02) * speed);

        // convert to screen space
        vec2 a = pos0;
        vec2 b = pos1;

        float d = lineDist(uv, a, b);

        float line = step(d, 0.002);

        float brightness = 0.6 + hash(fi * 9.1) * 0.6;

        col += vec3(line * brightness);
    }

    FragColor = vec4(col, 1.0);
}