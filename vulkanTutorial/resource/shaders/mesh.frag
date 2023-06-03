#version 450

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vCoord;
layout(location = 2) in vec3 vNormal;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform Light
{
    vec3 Direction;
    vec3 Color;
} lightUBO;

layout(set = 1, binding = 0) uniform sampler2D imageSampler;

void main() {
    vec4 texColor = vec4(vColor, 1.0) * texture(imageSampler, vCoord);
    vec3 N = normalize(vNormal);
    vec3 L = -normalize(lightUBO.Direction);
    float diffuse = max(dot(N, L), 0.0);
    outColor = diffuse * texColor;
}