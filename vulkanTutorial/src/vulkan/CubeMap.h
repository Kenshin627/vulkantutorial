#pragma once
#include "Image.h"
#include <vulkan/vulkan.hpp>

class CubeMap
{
public:
	void Create(Device& device, const std::vector<const char*>& paths);
	
	void Clear();
	void CreateSampler();
	void CreateDescriptor();
	vk::DescriptorImageInfo GetDescriptor() { return m_Descriptor; }
private:
	Device m_Device;
	Image m_Image;
	vk::Sampler m_Sampler;
	vk::DescriptorImageInfo m_Descriptor;
	int m_Width, m_Height, m_Channels;
};