#pragma once
#include "Device.h"
#include <vulkan/vulkan.hpp>

class SwapChain
{
public:
	void Init(const Device& device, const Window& window, bool vSync);
	~SwapChain();
	void Destroy();

private:
	Device m_Device;
	bool m_vSync;
	vk::SwapchainKHR m_SwapChain;
	vk::SurfaceCapabilitiesKHR m_Capabilities;
	vk::Format m_Format;
	vk::ColorSpaceKHR m_ColorSpace;
	vk::PresentModeKHR m_PresentMode;
	vk::Extent2D m_Extent;
	uint32_t m_ImageCount;
	std::vector<vk::Image> m_Images;
	std::vector<vk::ImageView> m_ImageViews;
};