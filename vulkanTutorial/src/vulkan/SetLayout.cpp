#include "../Core.h"
#include "SetLayout.h"

void BindingSetLayout::Create(const Device& device, const std::vector<SetLayoutBinding>& bindings)
{
	m_Device = device;
	m_BindingParams = bindings;
	m_BindingCount =static_cast<uint32_t>(m_BindingParams.size());
	m_Bindings.resize(m_BindingCount);
	m_PoolSizes.resize(m_BindingCount);

	for (uint32_t i = 0; i < m_BindingCount; i++)
	{
		SetLayoutBinding& currentBinding = m_BindingParams[i];

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
	setlayoutInfo.setBindingCount(m_BindingCount)
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
	
	for (const auto& b : m_BindingParams)
	{
		vk::WriteDescriptorSet descriptorWriter;
		descriptorWriter.sType = vk::StructureType::eWriteDescriptorSet;
		descriptorWriter.setDescriptorCount(b.DescriptorCount)
						.setDescriptorType(b.Type)
						.setDstArrayElement(0)
						.setDstBinding(b.Binding)
						.setDstSet(m_DescriptorSet);
		if (b.IsBuffer())
		{
			descriptorWriter.setPBufferInfo(&b.BufferInfo);
		}
		else
		{
			descriptorWriter.setPImageInfo(&b.ImageInfo);
		}
		writeDescriptorSets.emplace_back(descriptorWriter);
	}
	m_Device.GetLogicDevice().updateDescriptorSets(m_BindingCount, writeDescriptorSets.data(), 0, nullptr);
}