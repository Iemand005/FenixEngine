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

    float triLobe1 = cos(angle * 3.0) * 0.5;
    float triLobe2 = cos(angle * 2.0 + 0.5) * 0.15;
    float triLobe3 = sin(angle * 4.0 + 1.2) * 0.1;

    float ridge1 = abs(sin(angle * 7.0)) * 0.18;
    float ridge2 = abs(sin(angle * 11.0 + 1.0)) * 0.08;

    float rings1 = sin(aTexCoord.y * 3.5 * 6.2832) * 0.25;
    float rings2 = sin(aTexCoord.y * 7.0 * 6.2832 + 2.0) * 0.08;

    float slowWave = sin(time * 0.2 - aTexCoord.y * 4.0) * 0.35;
    float fastWave = sin(time * 0.4 - aTexCoord.y * 8.0) * 0.12;
    float squeeze  = sin(time * 0.1 - aTexCoord.y * 2.5) * 0.25;
    float radialPulse = sin(angle + time * 0.3) * 0.1 + 0.9;

    float dynamic = (slowWave + fastWave + squeeze) * wobbleAmount * radialPulse;

    float newRadius = 1.0 + triLobe1 + triLobe2 + triLobe3 + ridge1 + ridge2 + rings1 + rings2 + dynamic;
    newRadius = max(newRadius, 0.05);

    vec3 pos = aPos + aNormal * (1.0 - newRadius);

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
