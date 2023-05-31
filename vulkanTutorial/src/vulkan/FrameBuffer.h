#pragma once

#include "ImageView.h"
#include "Image.h"
#include <vector>
#include <vulkan/vulkan.hpp>

struct FrameBufferAttachment
{
	enum AttachmentType
	{
		Color,
		Depth
	};
	AttachmentType Type;
	Image Attachment;
};

class FrameBuffer
{
public:
	FrameBuffer& SetAttachments(std::vector<FrameBufferAttachment>& attachments);
	void Create(vk::Device device, uint32_t width, uint32_t height, vk::RenderPass renderPass);
	void Clear();
	vk::Framebuffer GetVkFrameBuffer() const { return m_VkFrameBuffer; }
	std::vector<vk::ImageView>& GetAttachments() { return m_Attachments; }
	FrameBufferAttachment& GetColorAttachment(uint32_t attachmentIndex) { return m_ColorAttachments[attachmentIndex]; }
	FrameBufferAttachment& GetDepthAttachment(uint32_t attachmentIndex) { return m_DepthAttachments[attachmentIndex]; }
private:
	uint32_t m_Width;
	uint32_t m_Height;
	vk::RenderPass m_RenderPass;
	std::vector<vk::ImageView> m_Attachments;
	std::vector<FrameBufferAttachment> m_ColorAttachments;
	std::vector<FrameBufferAttachment> m_DepthAttachments;
	vk::Device m_Device;
	vk::Framebuffer m_VkFrameBuffer;
};