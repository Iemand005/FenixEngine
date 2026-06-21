#version 330 core

out vec4 FragColor;

uniform vec2 resolution;

void main()
{
    float y = gl_FragCoord.y / resolution.y;

    float gradient = 1.0 - y;

    vec3 color = vec3(gradient);

    FragColor = vec4(color, 1.0);
}