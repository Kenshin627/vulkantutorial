#version 450

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D imageSampler;

void main() {
    outColor = texture(imageSampler, vCoord);
}