#version 450 core
#include "uniforms.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;   // Added color attribute
layout (location = 2) in vec3 aNormal;  // Moved normal to location 2

out vec3 FragPos;
out vec3 Normal;
out vec3 Color; // Pass color to fragment shader

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor; // Assign input color to output color
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
