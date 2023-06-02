#include "../Core.h"
#include "PipelineLayout.h"

void PipeLineLayout::Create(const Device& device, const std::vector<DescriptorSetLayoutCreateInfo>& setLayouts, const std::vector<vk::PushConstantRange>& pushConsnts)
{
	m_Device = device;
	vk::Device vkDevice = m_Device.GetLogicDevice();
	m_BindingParams = setLayouts;
	m_SetlayoutCount = setLayouts.size();
	m_SetLayouts.resize(m_SetlayoutCount);	
	for (uint32_t i = 0; i < m_SetlayoutCount; i++)
	{
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {};
		uint32_t bindingCount = setLayouts[i].Bindings.size();
		setLayoutBindings.resize(bindingCount);
		for (uint32_t j = 0; j < bindingCount; j++)
		{
			const DescriptorBinding& currentBinding = setLayouts[i].Bindings[j];
			setLayoutBindings[j].setBinding(currentBinding.Binding)
							    .setDescriptorCount(currentBinding.DescriptorCount)
							    .setDescriptorType(currentBinding.Type)
							    .setPImmutableSamplers(nullptr)
							    .setStageFlags(currentBinding.ShaderStage);
			m_PoolSizes.emplace_back(currentBinding.Type, setLayouts[i].SetCount);
		}
		vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
		setLayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
		setLayoutInfo.setBindingCount(setLayoutBindings.size())
				     .setPBindings(setLayoutBindings.data());
		VK_CHECK_RESULT(vkDevice.createDescriptorSetLayout(&setLayoutInfo, nullptr, &m_SetLayouts[i]));
	}
	vk::PipelineLayoutCreateInfo layoutInfo;
	layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	layoutInfo.setSetLayoutCount(m_SetLayouts.size())
			  .setPSetLayouts(m_SetLayouts.data())
			  .setPushConstantRangeCount(pushConsnts.size())
			  .setPPushConstantRanges(pushConsnts.data());
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createPipelineLayout(&layoutInfo, nullptr, &m_PipelineLayout));
}

void PipeLineLayout::BuildAndUpdateSet(vk::DescriptorPool pool)
{
	m_DescriptorSets.resize(m_SetlayoutCount);
	for (uint32_t i = 0; i < m_SetlayoutCount; i++)
	{
		m_DescriptorSets[i].SetLayout = m_SetLayouts[i];
		uint32_t setCount = m_BindingParams[i].SetCount;
		m_DescriptorSets[i].DescriptorSets.resize(setCount);
		vk::DescriptorSetAllocateInfo setInfo;
		setInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
		setInfo.setDescriptorPool(pool)
			   .setDescriptorSetCount(setCount)
			   .setPSetLayouts(&m_SetLayouts[i]);
		VK_CHECK_RESULT(m_Device.GetLogicDevice().allocateDescriptorSets(&setInfo, m_DescriptorSets[i].DescriptorSets.data()));
		for (uint32_t j = 0; j < setCount; j++)
		{
			uint32_t setBindingCount = m_BindingParams[i].SetWriteData[j].size();
			std::vector< vk::WriteDescriptorSet> writeSets;
			writeSets.resize(setBindingCount);
			for (uint32_t k = 0; k < setBindingCount; k++)
			{
				auto& binding = m_BindingParams[i].Bindings[k];
				auto& writeData = m_BindingParams[i].SetWriteData[j][k];
				writeSets[i].sType = vk::StructureType::eWriteDescriptorSet;
				writeSets[i].setDescriptorCount(binding.DescriptorCount)
							.setDescriptorType(binding.Type)
							.setDstArrayElement(0)
							.setDstBinding(binding.Binding)
							.setDstSet(m_DescriptorSets[i].DescriptorSets[j]);
				if (writeData.IsImage)
				{
					writeSets[i].setPImageInfo(&writeData.ImageInfo);
				}
				else
				{
					writeSets[i].setPBufferInfo(&writeData.BufferInfo);
				}
			}
			m_Device.GetLogicDevice().updateDescriptorSets(writeSets.size(), writeSets.data(), 0, nullptr);
		}
	}
}