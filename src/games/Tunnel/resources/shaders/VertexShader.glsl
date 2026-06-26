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
    float slowWave = sin(time * 2.0 - aTexCoord.y * 8.0) * 0.6;
    float fastWave = sin(time * 5.0 - aTexCoord.y * 16.0) * 0.3;
    float squeeze = sin(time * 1.5 - aTexCoord.y * 6.0) * 0.3;
    float radial = sin(aTexCoord.x * 6.2832 + time * 2.0) * 0.15 + 0.85;

    float wave = (slowWave + fastWave + squeeze) * wobbleAmount * radial;
    vec3 pos = aPos + aNormal * wave;

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
