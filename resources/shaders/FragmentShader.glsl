#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    vec3 n = normalize(normal);
vec3 lightDir = vec3(1,0,0);
    vec3 l = normalize(-lightDir);
float diff = max(dot(n, l), 0.0);
    
    vec3 ambient = vec3(0.1);
vec3 diffuse = vec3(1.0) * diff;

vec3 lighting = ambient + diffuse;

    FragColor = texture(ourTexture, TexCoord) * vec4(lighting, 1.0);
}