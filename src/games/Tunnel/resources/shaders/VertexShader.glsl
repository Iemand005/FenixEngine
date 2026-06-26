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
    vec3 pos = aPos;

    float wobbleX = sin(time * 2.0 + pos.z * 0.5) * wobbleAmount;
    float wobbleY = cos(time * 1.5 + pos.z * 0.3) * wobbleAmount;
    float wobbleZ = sin(time * 1.8 + pos.z * 0.7) * wobbleAmount;

    pos += vec3(wobbleX, wobbleY, wobbleZ);

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}
