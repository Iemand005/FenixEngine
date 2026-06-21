#version 330 core

out vec4 FragColor;

uniform vec2 resolution;

void main()
{
    float x = gl_FragCoord.x / resolution.x;

    float gradient = 1.0 - x;

    vec3 color = vec3(gradient);

    FragColor = vec4(color, 1.0);
}