#version 450
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aCoord;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vCoord;
layout(location = 2) out vec3 vNormal;


layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec3 Pos;
} ubo;

void main() {
    vCoord = aCoord;   
    vWorldPos = vec3(ubo.model * vec4(aPosition, 1.0));
    vNormal = mat3(transpose(inverse(ubo.model))) * aNormal;
    gl_Position = ubo.proj * ubo.view  * vec4(vWorldPos, 1.0);
}