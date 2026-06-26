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
uniform float roundness;
uniform float haustraStrength;
uniform float animSpeed;

void main()
{
    float angle = aTexCoord.x * 6.2832;
    float v = aTexCoord.y;

    float tri = max(cos(angle * 3.0), 0.0) * roundness;

    float hfPos = fract(v * 5.0);
    float hfDist = min(hfPos, 1.0 - hfPos) * 2.0;
    float haustra = -pow(1.0 - hfDist, 2.0) * haustraStrength;

    float pWave = sin(animSpeed * time * 0.3 - v * 2.0 * 3.14159);
    float peristalsis = -pow(max(pWave, 0.0), 4.0) * 0.65;
    float breath = sin(animSpeed * time * 0.1 - v * 1.0) * 0.15;
    float radialPulse = sin(angle + animSpeed * time * 0.2) * 0.06 + 0.94;

    float dynamic = (peristalsis + breath) * wobbleAmount * radialPulse;

    float newRadius = 1.5 + tri + haustra + dynamic;
    newRadius = max(newRadius, 0.1);

    vec3 pos = aPos + aNormal * (1.0 - newRadius);

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
