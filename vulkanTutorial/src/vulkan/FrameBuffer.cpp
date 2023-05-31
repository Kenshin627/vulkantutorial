#include "../Core.h"
#include "FrameBuffer.h"
#include <algorithm>

FrameBuffer& FrameBuffer::SetAttachments(std::vector<FrameBufferAttachment>& attachments)
{
	for (auto& attachment : attachments)
	{
		if (attachment.Type == FrameBufferAttachment::AttachmentType::Color) 
		{
			m_ColorAttachments.push_back(attachment);
		}
		else
		{
			m_DepthAttachments.push_back(attachment);
		}
	}
	m_Attachments.resize(attachments.size());
	std::transform(attachments.begin(), attachments.end(), m_Attachments.begin(), [](FrameBufferAttachment& fba) {
		return fba.Attachment.GetVkImageView();
	});
	return *this;
}

void FrameBuffer::Create(vk::Device device, uint32_t width, uint32_t height, vk::RenderPass renderPass)
{
	m_Device = device;
	m_RenderPass = renderPass;
	m_Width = width;
	m_Height = height;
	vk::FramebufferCreateInfo frameBufferInfo;
	frameBufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
	frameBufferInfo.setAttachmentCount(static_cast<uint32_t>(m_Attachments.size()))
				   .setHeight(m_Height)
				   .setWidth(m_Width)
				   .setLayers(1)
				   .setPAttachments(m_Attachments.data())
				   .setRenderPass(m_RenderPass);
	VK_CHECK_RESULT(m_Device.createFramebuffer(&frameBufferInfo, nullptr, &m_VkFrameBuffer));
}

void FrameBuffer::Clear()
{
	if (m_VkFrameBuffer)
	{
		m_Device.destroyFramebuffer(m_VkFrameBuffer, nullptr);
	}
	m_Attachments.clear();
	m_ColorAttachments.clear();
	m_DepthAttachments.clear();
}