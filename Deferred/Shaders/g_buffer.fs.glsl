#version 330 core
layout (location = 0) out vec3 gPos;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 fragPos;

void main()
{
    // 存储第一个G缓冲纹理中的片段位置向量
    gPos = fragPos;
    // 同样存储对每个逐片段法线到G缓冲中
    gNormal = normalize(vec3(0.f, 1.f, 0.f));
    // 和漫反射对每个逐片段颜色
    gAlbedoSpec.rgb = vec3(0.f, 0.5f, 0.f);
    // 存储镜面强度到gAlbedoSpec的alpha分量
    gAlbedoSpec.a = 1.f;
}