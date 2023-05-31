#pragma once
#include "Image.h"
#include <vulkan/vulkan.hpp>

class Texture
{
public:
	void Create(Device& device, const char* path, bool generateMipmaps);
	void Clear();
	uint32_t GetMipLevel() { return m_MipLevel; }
	Image& GetImage() { return m_Image; }
	void CreateSampler() { m_Image.CreateSampler(); }
	void CreateDescriptor() { m_Image.CreateDescriptor(); }
	vk::DescriptorImageInfo GetDescriptor() { return m_Image.GetDescriptor(); }
	vk::Sampler GetSampler() { return m_Image.GetSampler(); }
private:
	Image m_Image;
	uint32_t m_MipLevel;
	Device m_Device;
};