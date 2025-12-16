#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec2 TexCoord;

uniform sampler2D ourTexture;
  
// uniform vec4 ourColor; // we set this variable in the OpenGL code.

void main()
{
    // FragColor = vec4(vec3(1.0) * normal.x * 1 + 0.5, 1.0);
    // FragColor = texture(ourTexture, TexCoord) * vec4(vec3(1.0) * normal.x * 1 + 0.5, 1.0);
    // FragColor = vec4(TexCoord + 0.5, 0.0, 1.0);
    FragColor = texture(ourTexture, TexCoord) * vec4(vec3(1.0) * normal.x * 1 + 0.5, 1.0);
}