#version 330 core
// Multiple
layout (location = 0) out vec3 gPos;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 fragPos;
in vec2 texCoords;
in vec3 normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{
    gPos = fragPos;
    gNormal = normalize(normal);
    // And the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, texCoords).rgb;
    // Store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(texture_specular1, texCoords).r;
}