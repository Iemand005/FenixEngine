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
    vec2 center = resolution * 0.5;

    vec3 col = vec3(0.0);

    float t = time * 0.5;

    for (int i = 0; i < 200; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));

        float seed = hash(fi * 78.233);

        float speed = mix(80.0, 300.0, hash(fi * 91.7)); // FAST ALL STARS

        // distance from center (NO fract)
        float d  = seed + t * speed * 0.01;
        float d0 = seed + (t - 0.02) * speed * 0.01;

        // wrap manually but smoothly (no jumps in segment space)
        d  = mod(d, 1.5);
        d0 = mod(d0, 1.5);

        vec2 a = center + dir * d0 * (resolution.y * 0.6);
        vec2 b = center + dir * d  * (resolution.y * 0.6);

        float dist = lineDist(frag, a, b);

        // TRUE 1px line
        float star = step(dist, 0.5);

        float brightness = 0.6 + hash(fi * 9.1) * 0.7;

        col += vec3(star * brightness);
    }

    FragColor = vec4(col, 1.0);
}