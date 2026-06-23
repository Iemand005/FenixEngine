#version 330 core
out vec4 FragColor;
in vec3 Normal;

uniform vec3 debugColor; // set per-object, e.g. random/HSV from an index

void main()
{
    vec3 n = normalize(Normal);

    // fixed "headlamp" light direction, in view-ish space terms
    vec3 lightDir = normalize(vec3(0.4, 0.6, 1.0));

    float diff = max(dot(n, lightDir), 0.0);
    vec3 lighting = vec3(0.25) + diff * 0.75; // ambient + diffuse

    FragColor = vec4(debugColor * lighting, 1.0);
}