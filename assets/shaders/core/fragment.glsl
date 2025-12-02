#version 450 core
#include "uniforms.glsl"

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float AO;

out vec4 FragColor;

uniform sampler2D blockAtlas;

void main()
{
    // Sample texture from atlas
    vec4 texColor = texture(blockAtlas, TexCoord);

    // Discard fully transparent pixels (for future glass/leaves support)
    if (texColor.a < 0.1) {
        discard;
    }

    // Lighting calculation
    vec3 normalizedLight = normalize(-lightDir);
    float diff = max(dot(normalize(Normal), normalizedLight), 0.0);

    // Combine lighting with texture
    vec3 ambient = 0.3 * texColor.rgb;
    vec3 diffuse = diff * texColor.rgb;

    // Apply ambient occlusion
    vec3 finalColor = (ambient + diffuse) * AO;

    FragColor = vec4(finalColor, texColor.a);
}
