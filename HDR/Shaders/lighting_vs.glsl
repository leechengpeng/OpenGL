#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

void main()
{
    vs_out.FragPos   = vec3(model * vec4(aPos, 1.0));
    // camera在Cube内部，因此需要反转一下法向量
    vs_out.Normal    = normalize(transpose(inverse(mat3(model))) * -aNormal);
    vs_out.TexCoords = aTexCoords;
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}