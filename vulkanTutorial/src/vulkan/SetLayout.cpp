#include "../Core.h"
#include "SetLayout.h"

void BindingSetLayout::Create(const Device& device, const std::vector<SetLayoutBinding>& bindings)
{
	m_Device = device;
	//DescriptorSetlayoutBinding
	uint32_t bindingSize = bindings.size();
	m_Bindings.resize(bindingSize);
	for (uint32_t i = 0; i < bindingSize; i++)
	{
		SetLayoutBinding currentBinding = bindings[i];

		m_Bindings[i].setBinding(currentBinding.binding)
					 .setDescriptorCount(1)
					 .setDescriptorType(currentBinding.type)
					 .setPImmutableSamplers(nullptr)
					 .setStageFlags(currentBinding.shaderStage);
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