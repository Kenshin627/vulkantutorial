#version 450
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aCoord;
layout(location = 3) in vec3 aColor;
layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vCoord;
layout(location = 2) out vec3 vNormal;

layout(push_constant) uniform UniformPushConstant
{
    mat4 model;
} upc;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
} ubo;

void main() {
    vColor = aColor;
    vCoord = aCoord;   
    mat4 modelMat = ubo.model * upc.model;
    vNormal = mat3(transpose(inverse(modelMat))) * aNormal;
    gl_Position = ubo.proj * ubo.view * modelMat * vec4(aPosition, 1.0);
}