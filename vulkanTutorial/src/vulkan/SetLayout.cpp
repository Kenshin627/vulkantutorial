#include "../Core.h"
#include "SetLayout.h"

void BindingSetLayout::Create(const Device& device, const std::vector<SetLayoutBinding>& bindings)
{
	m_Device = device;
	//DescriptorSetlayoutBinding
	uint32_t bindingSize = bindings.size();
	m_Bindings.resize(bindingSize);
	m_PoolSizes.resize(bindingSize);
	for (uint32_t i = 0; i < bindingSize; i++)
	{
		SetLayoutBinding currentBinding = bindings[i];

		m_Bindings[i].setBinding(currentBinding.Binding)
					 .setDescriptorCount(currentBinding.DescriptorCount)
					 .setDescriptorType(currentBinding.Type)
					 .setPImmutableSamplers(nullptr)
					 .setStageFlags(currentBinding.ShaderStage);

		m_PoolSizes[i].setDescriptorCount(currentBinding.PoolSizeDescriptorCount)
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