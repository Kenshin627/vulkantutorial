#include "../Core.h"
#include "RenderPass.h"

void RenderPass::Create(Device& device, const std::vector<vk::AttachmentDescription>& attachments, const std::vector<vk::SubpassDescription>& subpass, const std::vector<vk::SubpassDependency>& dependencies, const std::vector<vk::ClearValue>& clearValues, vk::Rect2D renderArea)
{
	m_Device = device;
	m_ClearValues = clearValues;
	m_RenderArea = renderArea;
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
				  .setPAttachments(attachments.data())
				  .setDependencyCount(static_cast<uint32_t>(dependencies.size()))
				  .setPDependencies(dependencies.data())
				  .setSubpassCount(static_cast<uint32_t>(subpass.size()))
				  .setPSubpasses(subpass.data());
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createRenderPass(&renderPassInfo, nullptr, &m_RenderPass));
}

void RenderPass::BuildFrameBuffer(std::vector<std::vector<FrameBufferAttachment>>& attachments, uint32_t width, uint32_t height)
{
	uint32_t frameBufferCount = static_cast<uint32_t>(attachments.size());
	if (frameBufferCount > 1)
	{
		m_IsPresentPass = true;
	}
	m_FrameBuffers.resize(frameBufferCount);
	for (uint32_t i = 0; i < frameBufferCount; i++)
	{
		m_FrameBuffers[i].SetAttachments(attachments[i]).Create(m_Device.GetLogicDevice(), width, height, m_RenderPass);
	}
}

void RenderPass::Begin(vk::CommandBuffer command, uint32_t imageIndex, vk::Rect2D renderArea)
{
	vk::RenderPassBeginInfo renderPassBegin;
	renderPassBegin.sType = vk::StructureType::eRenderPassBeginInfo;
	renderPassBegin.setClearValueCount(static_cast<uint32_t>(m_ClearValues.size()))
				   .setPClearValues(m_ClearValues.data())
				   .setRenderPass(m_RenderPass)
				   .setRenderArea(renderArea);
	if (m_IsPresentPass)
	{
		renderPassBegin.setFramebuffer(m_FrameBuffers[imageIndex].GetVkFrameBuffer());
	}
	else 
	{
		renderPassBegin.setFramebuffer(m_FrameBuffers[0].GetVkFrameBuffer());
	}
	command.beginRenderPass(&renderPassBegin, vk::SubpassContents::eInline);
}

void RenderPass::End(vk::CommandBuffer command)
{
	command.endRenderPass();
}

void RenderPass::ReBuildFrameBuffer(std::vector<std::vector<FrameBufferAttachment>>& attachments, uint32_t width, uint32_t height)
{
	ClearFrameBuffer();
	BuildFrameBuffer(attachments, width, height);
}

void RenderPass::ClearFrameBuffer()
{
	if (!m_FrameBuffers.empty())
	{
		for (auto& fb : m_FrameBuffers) 
		{
			fb.Clear();
		}
	}
}

void RenderPass::Clear()
{
	ClearFrameBuffer();
	m_Device.GetLogicDevice().destroyRenderPass(m_RenderPass, nullptr);
}