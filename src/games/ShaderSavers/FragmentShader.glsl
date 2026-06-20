#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    // center + aspect
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    // --- rotating coordinate frame (slow drift, not swirl plasma) ---
    float a = t * 0.35;
    mat2 rot = mat2(cos(a), -sin(a), sin(a), cos(a));
    p = rot * p;

    // --- CRYSTAL GRID 1 (horizontal lattice) ---
    float gx = sin((p.x * 10.0) + t * 1.7);
    float gy = sin((p.y * 10.0) - t * 1.3);

    // --- CRYSTAL GRID 2 (diagonal shifted lattice) ---
    float gd = sin((p.x + p.y) * 8.0 + t * 2.2);

    // --- CRYSTAL GRID 3 (inverse diagonal phase) ---
    float gi = sin((p.x - p.y) * 9.0 - t * 1.9);

    // combine as interference field
    float v = gx * 0.35 + gy * 0.35 + gd * 0.2 + gi * 0.1;

    // convert to usable range
    v = v * 0.5 + 0.5;

    // sharpen into “crystal edges”
    float edge = smoothstep(0.45, 0.55, v);

    // moving energy bands (gives speed illusion)
    float bands = sin(v * 20.0 + t * 5.0);

    // final intensity
    float finalV = mix(v, bands * 0.5 + 0.5, 0.35);

    // hue driven by interference (not time directly → more “organic”)
    float hue = finalV + sin(t * 0.2) * 0.1;

    vec3 col = hsv2rgb(vec3(hue, 1.0, 1.0));

    // energy contrast shaping
    col *= edge + 0.3;

    // subtle vignette
    float d = length(p);
    col *= smoothstep(1.4, 0.2, d);

    FragColor = vec4(col, 1.0);
}