#pragma once
#include "Device.h"

#include <vector>
#include <vulkan/vulkan.hpp>

struct DescriptorBinding
{
	vk::DescriptorType Type;
	vk::ShaderStageFlags ShaderStage;
	uint32_t Binding;
	//vk::DescriptorBufferInfo BufferInfo;
	//vk::DescriptorImageInfo ImageInfo;
	uint32_t DescriptorCount = 1;
	/*bool IsBuffer() const
	{
		switch (Type)
		{
		case vk::DescriptorType::eUniformBuffer: return true;
		default:
			break;
		}
		return false;
	}*/
};

struct DescriptorWriteData
{
	vk::DescriptorBufferInfo BufferInfo;
	vk::DescriptorImageInfo ImageInfo;
	bool IsImage;
};

struct DescriptorSetLayoutCreateInfo
{
	std::vector<DescriptorBinding> Bindings;	
	uint32_t SetCount;
	std::vector<std::vector<DescriptorWriteData>> SetWriteData;
};

struct DescriptorSetLayoutSet
{
	vk::DescriptorSetLayout SetLayout;
	std::vector<vk::DescriptorSet> DescriptorSets;
};

class PipeLineLayout
{
public:
	void Create(const Device& device, const std::vector<DescriptorSetLayoutCreateInfo>& setLayouts, const std::vector<vk::PushConstantRange>& pushConsnts);
	void BuildAndUpdateSet(vk::DescriptorPool pool);
	std::vector<vk::DescriptorSetLayout> GetSetLayout() { return m_SetLayouts; }
	vk::PipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
	std::vector<vk::DescriptorPoolSize>& GetPoolSizes() { return m_PoolSizes; }
	vk::DescriptorSet GetDescriptorSet() { return m_DescriptorSet; }
private:
	Device m_Device;
	vk::PipelineLayout m_PipelineLayout;
	std::vector<vk::DescriptorSetLayout> m_SetLayouts;
	std::vector<DescriptorSetLayoutSet> m_DescriptorSets;
	std::vector<vk::DescriptorPoolSize> m_PoolSizes;
	vk::DescriptorSet m_DescriptorSet;
	std::vector<DescriptorSetLayoutCreateInfo> m_BindingParams;
	uint32_t m_SetlayoutCount;
};