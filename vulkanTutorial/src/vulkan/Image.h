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
	vk::DescriptorImageInfo GetDescriptor() { return m_Descriptor; }
	vk::Sampler GetSampler() { return m_Sampler; }
	void CreateSampler();
	void CreateDescriptor();
	void Clear();
	void QueryImageType();
	//TODO:temp
	void SetImage(vk::Image image) { m_VkImage = image; }
	void SetImageView(vk::ImageView imageView) { m_View = imageView; }
public:
	enum ImageType
	{
		Color = 0,
		Depth
	};
private:
	Device m_Device;
	vk::Image m_VkImage;
	vk::DeviceMemory m_Memory;
	vk::ImageView m_View;
	vk::Extent3D m_Size;
	vk::Format m_Format;
	uint32_t m_MipLevel;
	uint32_t m_Layers;
	vk::Sampler m_Sampler;
	vk::DescriptorImageInfo m_Descriptor;
	//TODO:: use framebuffer attachements category
	ImageType m_ImageType;
};