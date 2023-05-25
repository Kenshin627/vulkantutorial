#pragma once
#include "Device.h"
#include "CommandManager.h"
#include <vulkan/vulkan.hpp>

class Image
{
public:
	void Create(Device& device, CommandManager& commandManager, uint32_t mipLevel, vk::SampleCountFlagBits samplerCount, vk::ImageType type, vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vk::MemoryPropertyFlags memoryFlags, vk::ImageLayout initialLayout, vk::SharingMode sharingMode);
	void TransiationLayout(vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess, vk::ImageLayout srcLayout, vk::PipelineStageFlags dstStage, vk::AccessFlags dstAccess, vk::ImageLayout dstLayout, vk::ImageAspectFlags aspectFlags);
	void CopyBufferToImage(vk::Buffer srcBuffer, vk::Extent3D size, vk::ImageLayout layout);
	void GenerateMipMaps();
	vk::Image GetVkImage() { return m_VkImage; }
	uint32_t GetMiplevel() { return m_MipLevel; }
private:
	vk::Image m_VkImage;
	vk::DeviceMemory m_Memory;
	Device m_Device;
	vk::Extent3D m_Size;
	CommandManager m_CommandManager;
	uint32_t m_MipLevel;
};