#version 330 core
out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 viewPos;

void main()
{
    // 从G-Buffer中获取数据
    vec3 FragPos = texture(gPosition, texCoords).rgb;
    vec3 Normal  = texture(gNormal, texCoords).rgb;
    vec3 Albedo  = texture(gAlbedoSpec, texCoords).rgb;
    float Specular = texture(gAlbedoSpec, texCoords).a;

    // 然后和往常一样地计算光照
    vec3 lighting = Albedo * 0.1; // 硬编码环境光照分量
    vec3 v = normalize(viewPos - FragPos);
    {
        vec3 lightColor = vec3(1.f, 1.0f, 1.f);
        vec3 lightPos = vec3(0.f, 0.f, 0.f);
        // 漫反射
        vec3 l = normalize(lightPos - FragPos);
        vec3 diffuse = max(dot(Normal, l), 0.0) * Albedo * lightColor;
        lighting += diffuse;
    }

    FragColor = vec4(lighting, 1.0);
}