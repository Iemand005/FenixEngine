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

    float tri = max(cos(angle * 3.0), 0.0) * 0.15;

    float hf = abs(sin(v * 6.0 * 3.14159));
    float haustraFold  = -pow(hf, 0.6) * 0.65;
    float haustraPouch =  pow(abs(sin(v * 6.0 * 3.14159 - 1.57)), 0.6) * 0.3;

    float peristalsis = -pow(abs(sin(time * 0.2 - v * 1.5 * 3.14159)), 3.0) * 0.6;
    float wave = sin(time * 0.25 - v * 2.5) * 0.2;
    float radialPulse = sin(angle + time * 0.25) * 0.08 + 0.92;

    float dynamic = (peristalsis + wave) * wobbleAmount * radialPulse;

    float newRadius = 1.5 + tri + haustraFold + haustraPouch + dynamic;
    newRadius = max(newRadius, 0.15);

    vec3 pos = aPos + aNormal * (1.0 - newRadius);

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
