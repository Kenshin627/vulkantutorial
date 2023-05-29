#pragma once

#include "Device.h"
#include <vector>
#include <vulkan/vulkan.hpp>

struct SetLayoutBinding
{
	vk::DescriptorType type;
	vk::ShaderStageFlags shaderStage;
	uint32_t binding;
};
class BindingSetLayout
{
public:
	void Create(const Device& device, const std::vector<SetLayoutBinding>& bindings);
	vk::DescriptorSetLayout GetSetLayout() { return m_SetLayout; }
	vk::PipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
private:
	vk::DescriptorSetLayout m_SetLayout;
	vk::PipelineLayout m_PipelineLayout;
	std::vector < vk::DescriptorSetLayoutBinding> m_Bindings;
	Device m_Device;
};