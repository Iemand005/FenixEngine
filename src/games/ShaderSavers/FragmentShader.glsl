#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

float hash2(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    // center coords
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    float intensity = 0.0;

    // FEWER stars = no white screen
    for (int i = 0; i < 40; i++)
    {
        float fi = float(i);

        // stable random angle per star
        float a = hash(fi) * 6.2831853;
        vec2 dir = vec2(cos(a), sin(a));

        // speed variation
        float speed = 0.8 + hash(fi + 3.1) * 2.5;

        // looping lifetime (keeps stars cycling instead of accumulating)
        float life = fract(t * speed * 0.15 + hash(fi * 9.2));

        // push from center outward
        float dist = life * 2.5;

        vec2 pos = dir * dist;

        // pixel star shape (tight + crisp)
        float d = length(p - pos);

        float star = smoothstep(0.03, 0.0, d);

        // fade as it moves outward (prevents white buildup)
        float fade = 1.0 - life;

        intensity += star * fade * 0.5;
    }

    // HARD clamp so it NEVER whites out
    intensity = clamp(intensity, 0.0, 1.0);

    vec3 col = vec3(intensity);

    FragColor = vec4(col, 1.0);
}