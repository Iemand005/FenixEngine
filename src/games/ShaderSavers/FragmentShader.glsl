#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float noise(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    float n = noise(uv * 10.0 + time * 0.5);
    float heat = uv.y + n * 0.3 + time * 0.2;

    vec3 col = vec3(
        heat,
        heat * heat,
        heat * heat * 0.3
    );

    FragColor = vec4(col, 1.0);
}