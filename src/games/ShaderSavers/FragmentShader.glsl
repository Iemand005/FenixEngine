#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    float speed = time * 0.5;
    float depth = 20.0;

    vec3 col = vec3(0.0);

    for (float i = 1.0; i < 50.0; i++)
    {
        vec2 p = uv * i;

        float h = hash(floor(p * depth + speed));

        float star = smoothstep(0.99, 1.0, h);

        col += star / i;
    }

    FragColor = vec4(col, col.r, 1.0);
}