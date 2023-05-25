#include "../Core.h"
#include "Shader.h"
#include "../../utils/readFile.h"

Shader::Shader(vk::Device device, const std::string& path)
{
	m_Device = device;
	auto shaderCode = ReadFile(path);
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
	shaderInfo.setCodeSize(shaderCode.size())
		      .setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()));
	VK_CHECK_RESULT(device.createShaderModule(&shaderInfo, nullptr, &m_VkShader));
}

Shader::~Shader()
{
	Clear();
}

void Shader::Clear()
{
	m_Device.destroyShaderModule(m_VkShader, nullptr);
}

void Shader::SetPipelineShaderStageInfo(vk::ShaderStageFlagBits flag, const char* main)
{
	m_ShaderStage.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	m_ShaderStage.setModule(m_VkShader)
		         .setPName(main)
		         .setStage(flag);
}

