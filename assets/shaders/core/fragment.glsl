#version 450 core
#include "uniforms.glsl"

in vec3 FragPos;
in vec3 Normal;
in vec3 Color; // Receive color from vertex shader

out vec4 FragColor;

void main()
{
    vec3 normalizedLight = normalize(-lightDir);
    float diff = max(dot(normalize(Normal), normalizedLight), 0.0);
    
    // Use the interpolated Color from the vertex shader
    vec3 ambient = 0.2 * Color; 
    vec3 diffuse = diff * Color;
    
    FragColor = vec4(ambient + diffuse, 1.0);
}
