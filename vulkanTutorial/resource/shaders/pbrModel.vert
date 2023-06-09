#version 450
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aCoord;
layout(location = 3) in vec4 aTangent;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vCoord;
layout(location = 2) out vec3 vNormal;
layout(location = 3) out vec4 vTangent;


layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec3 Pos;
} ubo;

layout(set = 1, binding = 0) uniform UniformBufferModelMatrix {
    mat4 modelMatrix;
} modelUBO;

void main() {
    vCoord = aCoord;   
    vWorldPos = vec3(ubo.model * modelUBO.modelMatrix * vec4(aPosition, 1.0));
    mat4 inverseTranspose = transpose(inverse(ubo.model * modelUBO.modelMatrix));
    vNormal = mat3(inverseTranspose) * aNormal;
    vTangent =  inverseTranspose * aTangent;
    gl_Position = ubo.proj * ubo.view  * vec4(vWorldPos, 1.0);
}