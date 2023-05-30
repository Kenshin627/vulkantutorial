#include "../Core.h"
#include "FrameBuffer.h"

FrameBuffer& FrameBuffer::SetAttachment(const vk::ImageView& attachment)
{
	m_Attachments.push_back(attachment);
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
}