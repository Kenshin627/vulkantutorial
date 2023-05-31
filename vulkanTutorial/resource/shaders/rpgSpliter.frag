#version 450

layout(binding = 0) uniform sampler2D samplerColor;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;

void main()
{
	vec4 r = texture(samplerColor, inUV + vec2(0.05, 0.0));
	vec4 g = texture(samplerColor, inUV);
	vec4 b = texture(samplerColor, inUV - vec2(-0.05, 0.0));
	outFragColor = vec4(r.r,g.g, b.b, 1.0);
}