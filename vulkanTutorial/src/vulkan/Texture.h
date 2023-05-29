#pragma once
#include "Image.h"
#include <vulkan/vulkan.hpp>

class Texture
{
public:
	void Create(Device& device, const char* path, bool generateMipmaps);
	void CreateSampler();
	void CreateDescriptor();
	void Clear();
	uint32_t GetMipLevel() { return m_MipLevel; }
	Image& GetImage() { return m_Image; }
	vk::Sampler GetSampler() { return m_Sampler; }
	vk::DescriptorImageInfo& GetDescriptor() { return m_Descriptor; }
private:
	Image m_Image;
	vk::Sampler m_Sampler;
	uint32_t m_MipLevel;
	Device m_Device;
	vk::DescriptorImageInfo m_Descriptor;
};