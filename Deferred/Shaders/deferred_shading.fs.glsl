#version 330 core
out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

void main()
{
    // 从G-Buffer中获取数据
    vec3 FragPos = texture(gPosition, texCoords).rgb;
    vec3 Normal  = texture(gNormal, texCoords).rgb;
    vec3 Diffuse  = texture(gAlbedoSpec, texCoords).rgb;
    float Specular = texture(gAlbedoSpec, texCoords).a;

    // 然后和往常一样地计算光照
    vec3 lighting = Diffuse * 0.1; // 硬编码环境光照分量
    vec3 v = normalize(viewPos - FragPos);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // 漫反射
        vec3 l = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, l), 0.0) * Diffuse * lights[i].Color;
        // 高光
        vec3 h = normalize(l + v);
        float spec = pow(max(dot(Normal, h), 0.0), 16.0);
        vec3 specular = lights[i].Color * spec * Specular;
        // Attenuation
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
        diffuse *= attenuation;
        specular *= attenuation;
        lighting += diffuse + specular;
    }

    FragColor = vec4(lighting, 1.0);
}