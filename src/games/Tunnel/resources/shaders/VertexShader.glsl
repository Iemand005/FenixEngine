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

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    float angle = aTexCoord.x * 6.2832;
    float v = aTexCoord.y;

    float tri = max(cos(angle * 3.0), 0.0) * roundness;

    float randH = hash(vec2(floor(v * 4.0), 0));
    float haustraFreq = 3.5 + randH * 3.5;
    float haustraDepth = 0.3 + randH * 0.7;
    float hfPhase = sin(v * 0.7 + v * v * 0.01) * 0.25;
    float hfPos = fract(v * haustraFreq + hfPhase);
    float hfDist = min(hfPos, 1.0 - hfPos) * 2.0;
    float haustra = -pow(1.0 - hfDist, 2.0) * haustraStrength * haustraDepth;

    float bulgeRand = hash(vec2(floor(v * 2.5 + time * 0.02), 1));
    float bulgeAngle = max(cos(angle * 2.0 + bulgeRand * 6.28), 0.0);
    float bulgePos = pow(max(sin(v * 2.7 + time * 0.08 + bulgeRand * 3.0), 0.0), 2.0);
    float bulge = bulgePos * bulgeAngle * bulgeRand * 0.5;

    float gurgle1 = pow(max(sin(animSpeed * time * 1.5 - v * 7.0), 0.0), 10.0) * 0.35;
    float gurgle2 = pow(max(sin(animSpeed * time * 1.2 - v * 5.3 + 1.2), 0.0), 10.0) * 0.3;
    float gurgle3 = pow(max(sin(animSpeed * time * 0.9 - v * 6.7 + 3.1), 0.0), 10.0) * 0.25;
    float gurgle = (gurgle1 + gurgle2 + gurgle3) * wobbleAmount;

    float forwardWave = sin(animSpeed * time * 0.2 - v * 2.0 * 3.14159) * 0.25;
    float backwardWave = sin(animSpeed * time * 0.12 + v * 2.5) * 0.15;
    float slowBreath = sin(animSpeed * time * 0.05 - v * 0.8) * 0.2;
    float radialPulse = sin(angle + animSpeed * time * 0.15) * 0.06 + 0.94;

    float dynamic = (forwardWave + backwardWave + slowBreath) * wobbleAmount * radialPulse;

    float newRadius = 1.5 + tri + haustra + bulge + gurgle + dynamic;
    newRadius = max(newRadius, 0.1);

    vec3 pos = aPos + aNormal * (1.0 - newRadius);

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
