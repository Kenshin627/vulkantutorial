#pragma once
#include "Device.h"

#include <vector>
#include <vulkan/vulkan.hpp>

struct DescriptorBinding
{
	vk::DescriptorType Type;
	vk::ShaderStageFlags ShaderStage;
	uint32_t Binding;
	uint32_t DescriptorCount = 1;
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
	std::vector<vk::DescriptorSetLayout>& GetSetLayout() { return m_SetLayouts; }
	vk::PipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
	std::vector<vk::DescriptorPoolSize>& GetPoolSizes() { return m_PoolSizes; }
	vk::DescriptorSet GetDescriptorSet(uint32_t layoutIndex, uint32_t setIndex = 0) { return m_DescriptorSets[layoutIndex].DescriptorSets[setIndex]; }
	std::vector<vk::DescriptorSet>& GetDescriptorSets(uint32_t layoutIndex) { return m_DescriptorSets[layoutIndex].DescriptorSets; }
	uint32_t GetMaxSet() { return m_SetCount; }
private:
	Device m_Device;
	vk::PipelineLayout m_PipelineLayout;
	std::vector<vk::DescriptorSetLayout> m_SetLayouts;
	std::vector<DescriptorSetLayoutSet> m_DescriptorSets;
	std::vector<vk::DescriptorPoolSize> m_PoolSizes;
	std::vector<DescriptorSetLayoutCreateInfo> m_BindingParams;
	uint32_t m_SetlayoutCount;
	uint32_t m_SetCount;
};