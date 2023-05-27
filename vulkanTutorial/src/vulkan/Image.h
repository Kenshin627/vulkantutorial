#pragma once
#include "Device.h"
#include "CommandManager.h"
#include <vulkan/vulkan.hpp>

class Image
{
public:
	void Create(Device& device, uint32_t mipLevel, vk::SampleCountFlagBits samplerCount, vk::ImageType type, vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vk::MemoryPropertyFlags memoryFlags, vk::ImageLayout initialLayout, vk::SharingMode sharingMode, uint32_t arrayLayers, vk::ImageCreateFlags flag);
	void TransiationLayout(vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess, vk::ImageLayout srcLayout, vk::PipelineStageFlags dstStage, vk::AccessFlags dstAccess, vk::ImageLayout dstLayout, vk::ImageAspectFlags aspectFlags);
	void CopyBufferToImage(vk::Buffer srcBuffer, vk::Extent3D size, vk::ImageLayout layout);
	void GenerateMipMaps();
	void CreateImageView(vk::Format format, vk::ImageAspectFlags aspectFlag = vk::ImageAspectFlagBits::eColor, vk::ImageViewType viewType = vk::ImageViewType::e2D, vk::ComponentMapping mapping = vk::ComponentMapping());
	vk::Image GetVkImage() { return m_VkImage; }
	vk::ImageView GetVkImageView() { return m_View; }
	uint32_t GetMiplevel() { return m_MipLevel; }
	void Clear();
private:
	vk::Image m_VkImage;
	vk::DeviceMemory m_Memory;
	vk::ImageView m_View;
	Device m_Device;
	vk::Extent3D m_Size;
	uint32_t m_MipLevel;
	uint32_t m_Layers;
};