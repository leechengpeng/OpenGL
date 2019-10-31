#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 fragPos;
out vec2 texCoords;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    fragPos = vec3(model * vec4(aPosition, 1.0));
    texCoords = aTexCoords;
    normal = transpose(inverse(mat3(model))) * aNormal;

    gl_Position = projection * view * vec4(fragPos, 1.0);
}