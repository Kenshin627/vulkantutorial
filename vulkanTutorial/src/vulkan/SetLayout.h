#pragma once

#include "Device.h"
#include <vector>
#include <vulkan/vulkan.hpp>

struct SetLayoutBinding
{
	vk::DescriptorType Type;
	vk::ShaderStageFlags ShaderStage;
	uint32_t Binding;
	vk::DescriptorBufferInfo BufferInfo;
	vk::DescriptorImageInfo ImageInfo;
	uint32_t DescriptorCount = 1;
	bool IsBuffer() const
	{
		switch (Type)
		{
		case vk::DescriptorType::eUniformBuffer: return true;
		default:
			break;
		}
		return false;
	}
};
class BindingSetLayout
{
public:
	void Create(const Device& device, const std::vector<SetLayoutBinding>& bindings);
	void BuildAndUpdateSet(vk::DescriptorPool pool);
	vk::DescriptorSetLayout GetSetLayout() { return m_SetLayout; }
	vk::PipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
	std::vector<vk::DescriptorPoolSize>& GetPoolSizes() { return m_PoolSizes; }
	vk::DescriptorSet GetDescriptorSet() { return m_DescriptorSet; }
private:
	vk::DescriptorSetLayout m_SetLayout;
	vk::PipelineLayout m_PipelineLayout;
	std::vector<vk::DescriptorSetLayoutBinding> m_Bindings;
	std::vector<vk::DescriptorPoolSize> m_PoolSizes;
	vk::DescriptorSet m_DescriptorSet;
	std::vector<SetLayoutBinding> m_SeyLayoutBindings;
	Device m_Device;
};