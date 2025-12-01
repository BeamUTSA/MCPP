#version 450 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightDir;

void main()
{
    vec3 normalizedLight = normalize(-lightDir);
    float diff = max(dot(normalize(Normal), normalizedLight), 0.0);
    vec3 baseColor = vec3(0.4, 0.8, 0.4);
    vec3 ambient = 0.2 * baseColor;
    vec3 diffuse = diff * baseColor;
    FragColor = vec4(ambient + diffuse, 1.0);
}
