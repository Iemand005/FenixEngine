#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;

    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time * 0.6;

    float v =
        sin(p.x * 3.0 + t) +
        sin(p.y * 3.0 + t) +
        sin((p.x + p.y) * 3.0 + t) +
        sin(length(p) * 5.0 - t);

    v = v / 1.0;
    v = v * 0.5 + 0.5;

    float hue = v + time * 0.1;

    vec3 col = hsv2rgb(vec3(hue, 1.0, 1.0));

    FragColor = vec4(col, 1.0);
}