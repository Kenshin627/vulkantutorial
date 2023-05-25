#pragma once
#include <string>
#include <vulkan/vulkan.hpp>

class Shader
{
public:
	Shader(vk::Device device, const std::string& path);
	~Shader();
	void SetPipelineShaderStageInfo(vk::ShaderStageFlagBits flag, const char* main = "main");
	void Clear();
public:
	vk::PipelineShaderStageCreateInfo m_ShaderStage;
private:
	vk::ShaderModule m_VkShader;
	vk::Device m_Device;
};