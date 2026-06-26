#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float wobbleAmount;

void main()
{
    float angle = aTexCoord.x * 6.2832;
    float v = aTexCoord.y;

    float tri = cos(angle * 3.0) * 0.4;

    float haustra = -pow(abs(sin(v * 6.0 * 3.14159)), 0.25) * 1.0;

    float wave1 = sin(time * 0.3 - v * 3.0) * 0.35;
    float wave2 = sin(time * 0.15 - v * 1.2) * 0.55;
    float wave3 = sin(time * 0.4 - v * 5.0) * 0.2;
    float radialPulse = sin(angle + time * 0.35) * 0.1 + 0.9;

    float dynamic = (wave1 + wave2 + wave3) * wobbleAmount * radialPulse;

    float newRadius = 2.2 + tri + haustra + dynamic;
    newRadius = max(newRadius, 0.1);

    vec3 pos = aPos + aNormal * (1.0 - newRadius);

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
