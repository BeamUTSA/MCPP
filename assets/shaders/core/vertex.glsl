#version 450 core
#include "uniforms.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec2 aTileScale;
layout (location = 4) in float aAO;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float AO;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord * aTileScale;  // ← actually use tileScale!
    AO = aAO;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}