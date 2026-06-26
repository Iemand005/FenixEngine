#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(binding = 0) uniform UniformBuffer {
    mat4 projection;
    mat4 view;
    mat4 model;
    float time;
    float wobbleAmount;
} ubo;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    vec3 pos = position;

    float wobbleX = sin(ubo.time * 2.0 + pos.y * 0.5) * ubo.wobbleAmount;
    float wobbleY = cos(ubo.time * 1.5 + pos.y * 0.3) * ubo.wobbleAmount;
    float wobbleZ = sin(ubo.time * 1.8 + pos.y * 0.7) * ubo.wobbleAmount;

    pos += vec3(wobbleX, wobbleY, wobbleZ);

    float radiusWobble = 1.0 + sin(ubo.time * 2.5 + pos.y * 0.4) * 0.2;
    pos = normalize(pos) * radiusWobble;

    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos, 1.0);
    fragNormal = normal;
    fragTexCoord = texCoord;
}
