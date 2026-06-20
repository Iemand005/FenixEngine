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

    // center space
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    float col = 0.0;

    // sparse star population
    for (int i = 0; i < 50; i++)
    {
        float fi = float(i);

        // random direction per star
        float a = hash(fi) * 6.2831853;
        vec2 dir = vec2(cos(a), sin(a));

        // radial acceleration factor (KEY PART YOU ASKED FOR)
        float baseSpeed = 0.4 + hash(fi + 2.0) * 0.8;

        float accel = 1.0 + length(p) * 2.5; // faster further out

        float speed = baseSpeed * accel;

        // position expands from center
        float dist = fract(t * speed + hash(fi * 10.0)) * 3.0;

        vec2 pos = dir * dist;

        // vector to pixel
        vec2 d = p - pos;

        // NO BLUR, just hard pixel star
        float star = step(length(d), 0.02);

        // fade slightly with distance so edge isn't white wall
        float fade = 1.0 - dist * 0.25;

        col += star * fade;
    }

    col = clamp(col, 0.0, 1.0);

    FragColor = vec4(vec3(col), 1.0);
}