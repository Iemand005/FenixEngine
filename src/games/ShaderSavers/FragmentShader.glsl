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

    // center + aspect correction
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    // FAST MOTION BASE LAYERS (all cheap)
    float wave1 = sin(p.x * 6.0 + t * 2.5);
    float wave2 = cos(p.y * 6.0 - t * 2.2);
    float wave3 = sin((p.x + p.y) * 5.0 + t * 3.0);

    // diagonal sweep = motion illusion amplifier
    float sweep = sin((p.x * 2.0 + p.y * 2.0) + t * 4.0);

    // radial pulse (kept simple)
    float d = length(p);
    float radial = sin(d * 10.0 - t * 3.5);

    // combine (NO LOOPS, NO NOISE)
    float v = wave1 + wave2 + wave3 + sweep + radial;

    // normalize safely (5 layers → divide)
    v *= 0.2;
    v = v * 0.5 + 0.5;

    // sharpen motion contrast
    v = smoothstep(0.25, 0.75, v);

    // fast hue cycling
    float hue = v + t * 0.08;

    // color intensity boost for “speed feel”
    vec3 col = hsv2rgb(vec3(hue, 1.0, 1.0));

    // directional motion streak illusion (cheap trick)
    float streak = sin(p.x * 20.0 + t * 6.0) * 0.5 + 0.5;
    col += streak * 0.15;

    // vignette to focus motion center
    col *= smoothstep(1.3, 0.2, d);

    FragColor = vec4(col, 1.0);
}