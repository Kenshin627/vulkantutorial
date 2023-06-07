#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vCoord;
layout(location = 2) in vec3 vNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec3 Pos;
} ubo;

struct light
{
    vec4 Pos;
    vec4 Color;
};

layout(set = 0, binding = 1) uniform UniformLight
{
    light lights[4];
} lightUBO;

layout(push_constant) uniform Material
{
    layout(offset = 12) float roughness;
    layout(offset = 16) float metallic;
    layout(offset = 20) float r;
    layout(offset = 24) float g;
    layout(offset = 28) float b;
} material;

const float PI = 3.14159265358979;

vec3 baseColor()
{
    return vec3(material.r, material.g, material.b);
}

float pow5(float x)
{
    return x * x * x * x * x;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow5(clamp(1.0 - cosTheta, 0.0, 1.0));
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

//vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
//{
//    
//}
//
void main() {
    vec3 albedo =  baseColor();
   vec3 N = normalize(vNormal);
   vec3 V = normalize(ubo.Pos - vWorldPos);

   vec3 Lo = vec3(0.0);
   for(int i = 0; i < 4; i++)
   {
         vec3 L = normalize(lightUBO.lights[i].Pos.xyz - vWorldPos);
         vec3 H = normalize(L + V);
         float distance = length(lightUBO.lights[i].Pos.xyz - vWorldPos);
         float attenuation = 1.0 / (distance * distance);
         vec3 radiance =  lightUBO.lights[i].Color.rgb;
         vec3 F0 = vec3(0.04);
         F0 = mix(F0, albedo, material.metallic);
         vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);
         float NDF = DistributionGGX(N, H, material.roughness);
         float G = GeometrySmith(N, V, L, material.roughness);
         vec3 numerator = NDF * G * F;
         float demominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
         vec3 specular = numerator / demominator;

         vec3 kS = F;
         vec3 kD = vec3(1.0) - F;
         kD *= 1.0 - material.metallic;

         float NDotL = max(dot(N, L), 0.0);
         Lo += (kD * albedo / PI + specular) * radiance * NDotL;
   }

   vec3 ambient = vec3(0.03) * albedo;
   vec3 color = ambient + Lo;
   color  = pow(color, vec3(0.4545));
   outColor = vec4(color, 1.0);
}