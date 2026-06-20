#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

void main()
{
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    float r = length(uv);
    float a = atan(uv.y, uv.x);

    float tunnel = sin(10.0 * r - time * 3.0 + a * 2.0);

    vec3 col = vec3(
        sin(tunnel + time),
        sin(tunnel + 2.0),
        sin(tunnel + 4.0)
    );

    FragColor = vec4(col * 0.5 + 0.5, 1.0);
}