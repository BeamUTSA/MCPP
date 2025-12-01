#ifndef UNIFORMS_GLSL
#define UNIFORMS_GLSL

// Uniforms
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 lightDir;

// SSBO for Block Data (if needed later, currently not used in shaders)
// struct Block {
//     vec2 faces[6];
// };
// layout(std430, binding = 0) buffer BlockData {
//     Block blocks[];
// };

#endif // UNIFORMS_GLSL
