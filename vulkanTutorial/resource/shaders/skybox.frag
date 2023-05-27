#version 450

layout(location = 0) in vec3 vCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 2) uniform samplerCube imageSampler;

void main() {
    outColor = texture(imageSampler, vCoord);
}