#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// --- hash / noise helpers ---
float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
           (c - a) * u.y * (1.0 - u.x) +
           (d - b) * u.x * u.y;
}

// fbm for layered motion
float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;

    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);

    for (int i = 0; i < 5; i++)
    {
        v += a * noise(p);
        p = m * p;
        a *= 0.5;
    }
    return v;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 center = uv - 0.5;

    float aspect = resolution.x / resolution.y;
    center.x *= aspect;

    float t = time * 0.8;

    // --- radial warp (gives speed feeling) ---
    float r = length(center);
    float ang = atan(center.y, center.x);

    ang += t * 1.5;
    r += fbm(center * 3.0 + t) * 0.2;

    vec2 warped;
    warped.x = cos(ang) * r;
    warped.y = sin(ang) * r;

    // --- plasma field ---
    float field = fbm(warped * 4.0 + vec2(t * 2.0, -t * 1.5));

    // --- fast streak motion layer ---
    float streaks = fbm(vec2(
        warped.y * 6.0 + t * 5.0,
        warped.x * 6.0 - t * 4.0
    ));

    // --- glow core ---
    float core = smoothstep(0.35, 0.0, length(center));

    // --- color palette (hot neon plasma) ---
    vec3 col = vec3(0.0);

    col += vec3(0.2, 0.6, 1.0) * field;
    col += vec3(1.0, 0.3, 0.8) * streaks * 0.6;
    col += vec3(1.0, 0.9, 0.2) * core;

    // extra contrast pop
    col = pow(col, vec3(1.3));

    // motion emphasis (slight flicker for high refresh feel)
    col += 0.05 * sin(time * 30.0 + uv.xyx * 10.0);

    FragColor = vec4(col, 1.0);
}