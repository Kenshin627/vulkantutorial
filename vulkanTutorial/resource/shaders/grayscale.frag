#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 outColor;

void main()
{
//	outColor = vec4(0.2, 0.8, 0.3, 1.0);
	outColor = vec4(subpassLoad(inputColor).rgb, 1.0);
}