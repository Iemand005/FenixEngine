#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// stable hash
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

    // centered coords
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    vec3 col = vec3(0.0);

    // number of stars (NOT a grid — just deterministic sampling slices)
    for (int i = 0; i < 60; i++)
    {
        float fi = float(i);

        // each star has a stable random direction
        float a = hash(fi) * 6.2831853;

        vec2 dir = vec2(cos(a), sin(a));

        // speed variation
        float speed = 1.5 + hash(fi + 10.0) * 6.0;

        // star position = moves outward continuously from center
        float life = fract(t * speed * 0.05 + hash(fi * 2.0));

        float dist = life * 3.0;

        vec2 pos = dir * dist;

        // vector from star to pixel
        vec2 diff = p - pos;

        float d = length(diff);

        // motion streak (projection onto direction)
        float along = abs(dot(diff, dir));

        float streak = 1.0 / (along * 25.0 + 1.0);

        // core star
        float core = smoothstep(0.05, 0.0, d);

        float intensity = (core + streak * 0.8);

        // fade with distance from center (burst origin)
        intensity *= smoothstep(3.0, 0.2, length(pos));

        col += vec3(intensity);
    }

    // tone down brightness
    col *= 0.8;

    FragColor = vec4(col, 1.0);
}