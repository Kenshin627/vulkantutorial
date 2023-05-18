#version 450
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 0) out vec3 vColor;
layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
} ubo;

void main() {
    vColor = aColor;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(aPosition, 0.0, 1.0);
}