#pragma once

#include "ImageView.h"
#include <vector>
#include <vulkan/vulkan.hpp>

class FrameBuffer
{
public:
	FrameBuffer& SetAttachment(const vk::ImageView& attachment);
	void Create(vk::Device device, uint32_t width, uint32_t height, vk::RenderPass renderPass);
	void Clear();
	vk::Framebuffer GetVkFrameBuffer() const { return m_VkFrameBuffer; }
private:
	uint32_t m_Width;
	uint32_t m_Height;
	vk::RenderPass m_RenderPass;
	std::vector<vk::ImageView> m_Attachments;
	vk::Device m_Device;
	vk::Framebuffer m_VkFrameBuffer;
};