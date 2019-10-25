#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;

void main()
{
    // TODO: 学习法线矩阵，http://www.lighthouse3d.com/tutorials/glsl-tutorial/the-normal-matrix/
    // Normal = mat3(transpose(inverse(model))) * aNormal;

    fragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(fragPos, 1.0);
}