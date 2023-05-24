#pragma once
#include "Device.h"
#include <vulkan/vulkan.hpp>

class SwapChain
{
public:
	SwapChain() = default;
	void Init(const Device& device, const Window& window, bool vSync);
	void Create();
	void ReCreate();
	~SwapChain();
	void Clear();
	vk::SwapchainKHR GetSwapChain() { return m_SwapChain; }
	vk::Format GetFormat() { return m_Format; }
	vk::ColorSpaceKHR GetColorSpace() { return m_ColorSpace; }
	vk::Extent2D GetExtent() { return m_Extent; }
	std::vector<vk::Image> GetImages() { return m_Images; }
	std::vector<vk::ImageView> GetImageViews() { return m_ImageViews; }
private:
	Device m_Device;
	bool m_vSync;
	Window m_Window;
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