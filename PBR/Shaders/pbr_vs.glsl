#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    TexCoords = aTexCoords;
    WorldPos  = vec3(uModel * vec4(aPos, 1.0));
    Normal    = mat3(uModel) * aNormal;   

    gl_Position =  uProjection * uView * vec4(WorldPos, 1.0);
}