#pragma once

#include "Image.h"
#include "Device.h"
#include "FrameBuffer.h"

#include <vector>
#include <vulkan/vulkan.hpp>

class RenderPass
{
public:
	void Create(Device& device, const std::vector<vk::AttachmentDescription>& attachments, const std::vector<vk::SubpassDescription>& subpass, const std::vector<vk::SubpassDependency>& dependencies, const std::vector<vk::ClearValue>& clearValues, vk::Rect2D renderArea);
	void BuildFrameBuffer(std::vector<std::vector<FrameBufferAttachment>>& attachments, uint32_t width, uint32_t height);
	void ReBuildFrameBuffer(std::vector<std::vector<FrameBufferAttachment>>& attachments, uint32_t width, uint32_t height);
	void ClearFrameBuffer();
	void SetRenderArea(vk::Rect2D renderArea) { m_RenderArea = renderArea; }
	void Begin(vk::CommandBuffer command, uint32_t imageIndex, vk::Rect2D renderArea);
	void Clear();
	void End(vk::CommandBuffer command);
	vk::RenderPass GetVkRenderPass() { return m_RenderPass; }
	std::vector<FrameBuffer>& GetFrameBuffers() { return m_FrameBuffers; }

private:
	Device m_Device;
	vk::RenderPass m_RenderPass;
	std::vector<FrameBuffer> m_FrameBuffers;
	std::vector<vk::ClearValue> m_ClearValues;
	vk::Rect2D m_RenderArea;
	bool m_IsPresentPass = false;
};