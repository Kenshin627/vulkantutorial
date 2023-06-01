#version 450

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vCoord;
layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 0) uniform sampler2D imageSampler;

void main() {
    outColor = vec4(vColor, 1.0) * texture(imageSampler, vCoord);
}