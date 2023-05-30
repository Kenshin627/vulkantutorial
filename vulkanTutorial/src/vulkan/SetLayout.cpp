#include "../Core.h"
#include "SetLayout.h"

void BindingSetLayout::Create(const Device& device, const std::vector<SetLayoutBinding>& bindings)
{
	m_Device = device;
	m_SeyLayoutBindings = bindings;
	uint32_t bindingSize = m_SeyLayoutBindings.size();
	m_Bindings.resize(bindingSize);
	m_PoolSizes.resize(bindingSize);

	for (uint32_t i = 0; i < bindingSize; i++)
	{
		SetLayoutBinding& currentBinding = m_SeyLayoutBindings[i];

		m_Bindings[i].setBinding(currentBinding.Binding)
					 .setDescriptorCount(currentBinding.DescriptorCount)
					 .setDescriptorType(currentBinding.Type)
					 .setPImmutableSamplers(nullptr)
					 .setStageFlags(currentBinding.ShaderStage);

		m_PoolSizes[i].setDescriptorCount(currentBinding.DescriptorCount)
				      .setType(currentBinding.Type);
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
	std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
	uint32_t bindingSize = m_SeyLayoutBindings.size();
	writeDescriptorSets.resize(bindingSize);
	for (uint32_t i = 0; i < bindingSize; i++)
	{
		SetLayoutBinding& currentBinding = m_SeyLayoutBindings[i];
		writeDescriptorSets[i].sType = vk::StructureType::eWriteDescriptorSet;
		writeDescriptorSets[i].setDescriptorCount(currentBinding.DescriptorCount)
							  .setDescriptorType(currentBinding.Type)
							  .setDstArrayElement(0)
							  .setDstBinding(currentBinding.Binding)
							  .setDstSet(m_DescriptorSet);
		if (currentBinding.IsBuffer())
		{
			writeDescriptorSets[i].setPBufferInfo(&currentBinding.BufferInfo);
		}
		else
		{
			writeDescriptorSets[i].setPImageInfo(&currentBinding.ImageInfo);
		}
	}
	m_Device.GetLogicDevice().updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}