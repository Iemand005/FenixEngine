#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;
uniform float wobbleAmount;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    vec3 pos = position;

    float wobbleX = sin(time * 2.0 + pos.y * 0.5) * wobbleAmount;
    float wobbleY = cos(time * 1.5 + pos.y * 0.3) * wobbleAmount;
    float wobbleZ = sin(time * 1.8 + pos.y * 0.7) * wobbleAmount;

    pos += vec3(wobbleX, wobbleY, wobbleZ);

    gl_Position = projection * view * model * vec4(pos, 1.0);
    fragNormal = normal;
    fragTexCoord = texCoord;
}
