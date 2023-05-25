#pragma once
#include <vulkan/vulkan.hpp>

class ImageView
{
public:
	void Create(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlag = vk::ImageAspectFlagBits::eColor, uint32_t mipLevels = 1, vk::ImageViewType viewType = vk::ImageViewType::e2D, vk::ComponentMapping mapping = vk::ComponentMapping());
	vk::ImageView GetVkImageView() const { return m_View; }
	void Clear();
private:
	vk::Device m_Device;
	vk::ImageView m_View;
	vk::Image m_Image;
};