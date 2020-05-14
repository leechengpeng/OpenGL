#version 330 core

out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;

// material parameters
uniform vec3  uAlbedo;
uniform float uMetallic;
uniform float uRoughness;

// IBL
uniform samplerCube uIrradianceMap;

// lights
uniform vec3 uLightPos[4];
uniform vec3 uLightColor[4];

// other
uniform vec3 uCamPos;

const float PI = 3.14159265359;
const float EPSINON = 0.0000001;

// NDF, Normal Distribution Function：粗糙度越大，微平面取向越随机，集中性（高亮）降低，最终效果越发灰暗
float DistributionGGX(float NoH, float roughness) 
{
    float alpha = roughness * roughness;
    float a2 = alpha * alpha;

    // 此处有指令优化空间，但是为了便于理解暂不做优化，后续可尝试
    float denom = (NoH * NoH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, EPSINON); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

// Geometry Function：从统计学上近似的求得微表面之间的相互遮蔽比率，相互遮挡会损耗光线的能量
float GeometrySmith(float NoV, float NoL, float roughness)
{
    // remapping
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    // Geometry Obstruction + Geometry Shadowing
    float ggx1 = NoV / (NoV * (1.0 - k) + k);
    float ggx2 = NoL / (NoL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

// Fresnel Function：
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Bidirectional Reflective Distribution Function
vec3 brdf(vec3 Wi, vec3 Wo, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic)
{
    vec3  H   = normalize(Wi + Wo);
    float NoH = max(dot(N, H),  0.0);
    float NoV = max(dot(N, Wo), 0.0);
    float NoL = max(dot(N, Wi), 0.0);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(NoH, roughness);   
    float G   = GeometrySmith(NoV, NoL, roughness);      
    vec3  F   = FresnelSchlick(NoH, F0);
    vec3  nominator   = NDF * G * F; 
    float denominator = 4 * max(dot(N, Wo), 0.0) * max(dot(N, Wi), 0.0);
    vec3  specular    = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
        
    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    return kD * albedo / PI + specular; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}

void main()
{
    vec3  albedo    = uAlbedo;
    float metallic  = uMetallic;
    float roughness = uRoughness;
    float ao        = 1.0;

    vec3 N = normalize(Normal);
    vec3 V = normalize(uCamPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo =- vec3(0.0);
    for (int i = 0; i < 4; ++i)
    {
        // cal per-light radiance
        float distance = length(uLightPos[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = uLightColor[i] * attenuation;

        vec3 L = normalize(uLightPos[i] - WorldPos);
        vec3 fr = brdf(L, V, N, F0, albedo, roughness, metallic);
        Lo += fr * radiance * max(dot(N, L), 0.0);
    }
    
    // ambient lighting (we now use IBL as the ambient term)
    vec3 kS = FresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = texture(uIrradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    vec3 ambient = (kD * diffuse) * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}