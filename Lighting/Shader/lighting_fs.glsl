#version 330 core
in vec3 fragPos;

out vec4 FragColor;

vec3 lightColor = vec3(1.f, 1.0f, 1.f);
vec3 lightPos = vec3(0.f, 1.f, 0.f);

uniform vec3 camPosition;

void main()
{
    // 待渲染的平面是xz平面，为简化代码直接将法线设置成朝向y轴
    vec3 n = vec3(0.f, 1.f, 0.f);

    // 环境光照（全局光照的简单替代品）
    float ambientStrength = 0.1f;
	float ambient = ambientStrength;

    // 漫反射
    vec3 l = normalize(lightPos - fragPos);
    float diffuse = max(dot(n, l), 0.f);

    // 镜面发射
    vec3 v = normalize(camPosition - fragPos);
    float specStrength = 1.f;
    float specular = 0.0;
    // Phong
    bool phong_model = false;
    if (phong_model)
    {
        vec3 r = reflect(-l, n);
        specular = specStrength * pow(max(dot(v, r), 0.f), 8);
    }
    // Blinn-Phong
    else
    {
        vec3 h = normalize(l + v);
        specular = specStrength * pow(max(dot(n, h), 0.0), 32.0);
    }

    // FragColor = vec4((ambient + diffuse + specular) * lightColor, 1.0);
    FragColor = vec4((ambient + specular) * lightColor, 1.0);
}