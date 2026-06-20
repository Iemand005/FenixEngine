#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// simple hash (deterministic pseudo-random)
float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    // center space (optional aesthetic; keeps aspect stable)
    vec2 p = uv;
    p.x *= resolution.x / resolution.y;

    // STARFIELD SPACE SCALE (density control)
    float scale = 60.0;
    vec2 sp = p * scale;

    // FAST MOTION: warp flight downward + slight drift
    float t = time;

    sp.y += t * 6.0;
    sp.x += t * 1.5;

    // integer grid cell
    vec2 cell = floor(sp);
    vec2 f = fract(sp) - 0.5;

    // random per cell
    float rnd = hash(cell);

    // star existence probability
    float starMask = step(0.985, rnd); // fewer = more empty space

    // star shape (tight point)
    float d = length(f);

    // crisp star core
    float star = smoothstep(0.12, 0.0, d);

    // optional twinkle (cheap temporal variation)
    float twinkle = 0.6 + 0.4 * sin(t * 10.0 + rnd * 100.0);

    star *= twinkle;

    // combine
    float intensity = star * starMask;

    vec3 col = vec3(intensity);

    FragColor = vec4(col, 1.0);
}