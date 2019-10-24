#version 330 core
in vec3 fragPos;

out vec4 FragColor;

vec3 lightColor = vec3(1.f, 1.0f, 1.f);
vec3 lightPos = vec3(0.f, 1.f, -3.f);

uniform vec3 camPosition;

void main()
{
    // 待渲染的平面是xz平面，为简化代码直接将法线设置成朝向y轴
    vec3 n = vec3(0.f, 1.f, 0.f);

    // 环境光照（全局光照的简单替代品）
    float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * lightColor;

    // 漫反射
    vec3 l = normalize(lightPos - fragPos);
    vec3 diffuse = max(dot(n, l), 0.f) * lightColor;

    // 镜面发射
    vec3 v = normalize(camPosition - fragPos);
    vec3 r = reflect(-l, n);
    float specStrength = 0.5f;
    vec3 specular = specStrength * pow(max(dot(v, r), 0.f), 4) * lightColor;

    FragColor = vec4((ambient + diffuse + specular), 1.0);
    //FragColor = vec4((ambient + diffuse), 1.0);
}