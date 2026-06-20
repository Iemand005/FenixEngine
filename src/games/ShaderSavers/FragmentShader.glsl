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

    // centered coords
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time * 0.8;

    // simple rotation (cheap + stable)
    float c = cos(t * 0.6);
    float s = sin(t * 0.6);
    p = mat2(c, -s, s, c) * p;

    // radial distance
    float d = length(p);

    // SAFE plasma (no loops, no fbm)
    float v =
        sin(p.x * 4.0 + t) +
        cos(p.y * 4.0 - t) +
        sin((p.x + p.y) * 3.0 + t * 0.7) +
        cos(d * 8.0 - t * 1.3);

    // normalize safely (4 terms)
    v *= 0.25;
    v = v * 0.5 + 0.5;

    // smooth banding for motion clarity
    v = smoothstep(0.2, 0.8, v);

    // hue drift
    float hue = v + t * 0.05;

    // slight vignette (very cheap)
    float vignette = smoothstep(1.2, 0.2, d);

    vec3 col = hsv2rgb(vec3(hue, 1.0, 1.0));

    col *= vignette;

    FragColor = vec4(col, 1.0);
}