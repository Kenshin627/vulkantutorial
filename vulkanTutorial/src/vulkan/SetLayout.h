#pragma once

#include "Device.h"
#include <vector>
#include <vulkan/vulkan.hpp>

struct SetLayoutBinding
{
	vk::DescriptorType Type;
	vk::ShaderStageFlags ShaderStage;
	uint32_t Binding;
	uint32_t DescriptorCount = 1;
	uint32_t PoolSizeDescriptorCount = 1;
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
	std::vector<vk::DescriptorSetLayoutBinding> m_Bindings;
	std::vector<vk::DescriptorPoolSize> m_PoolSizes;
	
	Device m_Device;
};