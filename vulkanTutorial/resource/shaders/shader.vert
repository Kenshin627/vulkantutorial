#version 450

layout(binding = 0) uniform uniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aCoord;

layout(location = 0) out vec3 v_Color;
layout(location = 1) out vec2 v_Coord;

void main() {
    v_Color = aColor;
    v_Coord = aCoord;
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(aPosition, 0.0, 1.0);
}