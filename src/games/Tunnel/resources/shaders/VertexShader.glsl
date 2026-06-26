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
    float wave = sin(time * 3.0 - aTexCoord.y * 12.0) * wobbleAmount;
    vec3 pos = aPos + aNormal * wave;

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
