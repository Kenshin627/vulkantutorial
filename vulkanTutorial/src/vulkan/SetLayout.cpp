#include "../Core.h"
#include "SetLayout.h"

void BindingSetLayout::Create(const Device& device, const std::vector<SetLayoutBinding>& bindings)
{
	m_Device = device;
	uint32_t bindingSize = bindings.size();
	m_Bindings.resize(bindingSize);
	m_PoolSizes.resize(bindingSize);
	m_SetWrites.resize(bindingSize);
	for (uint32_t i = 0; i < bindingSize; i++)
	{
		SetLayoutBinding currentBinding = bindings[i];

		m_Bindings[i].setBinding(currentBinding.Binding)
					 .setDescriptorCount(currentBinding.DescriptorCount)
					 .setDescriptorType(currentBinding.Type)
					 .setPImmutableSamplers(nullptr)
					 .setStageFlags(currentBinding.ShaderStage);

		m_PoolSizes[i].setDescriptorCount(currentBinding.DescriptorCount)
				      .setType(currentBinding.Type);

		m_SetWrites[i].sType = vk::StructureType::eWriteDescriptorSet;
		m_SetWrites[i].setDescriptorCount(currentBinding.DescriptorCount)
					  .setDescriptorType(currentBinding.Type)
					  .setDstArrayElement(0)
					  .setDstBinding(currentBinding.Binding)
					  .setDstSet(m_DescriptorSet);
		if (currentBinding.IsBuffer())
		{
			m_SetWrites[i].setPBufferInfo(&currentBinding.BufferInfo);
		}
		else 
		{
			m_SetWrites[i].setPImageInfo(&currentBinding.ImageInfo);
		}
	}

	//SetLayout
	vk::DescriptorSetLayoutCreateInfo setlayoutInfo;
	setlayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
	setlayoutInfo.setBindingCount(bindingSize)
				 .setPBindings(m_Bindings.data());
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createDescriptorSetLayout(&setlayoutInfo, nullptr, &m_SetLayout));

	//PipelineLayout
	vk::PipelineLayoutCreateInfo layoutInfo;
	layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	layoutInfo.setSetLayoutCount(1)
			  .setPSetLayouts(&m_SetLayout)
			  .setPushConstantRangeCount(0)
			  .setPPushConstantRanges(nullptr);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createPipelineLayout(&layoutInfo, nullptr, &m_PipelineLayout));
}

void BindingSetLayout::BuildAndUpdateSet(vk::DescriptorPool pool)
{
	vk::DescriptorSetAllocateInfo setInfo;
	setInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
	setInfo.setDescriptorPool(pool)
		   .setDescriptorSetCount(1)
		   .setPSetLayouts(&m_SetLayout);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().allocateDescriptorSets(&setInfo, &m_DescriptorSet));
	for (auto& write : m_SetWrites)
	{
		write.setDstSet(m_DescriptorSet);
	}
	m_Device.GetLogicDevice().updateDescriptorSets(m_SetWrites.size(), m_SetWrites.data(), 0, nullptr);
}