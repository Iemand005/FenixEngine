#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform vec3 lightDir;

void main()
{
    vec3 n = normalize(Normal);
    vec3 l = normalize(-lightDir);
    float diff = max(dot(n, l), 0.0);
    
    vec3 ambient = vec3(0.1);
    vec3 diffuse = vec3(1.0) * diff;

vec3 lighting = ambient + diffuse;

    FragColor = texture(ourTexture, TexCoord) * vec4(lighting, 1.0);
}
