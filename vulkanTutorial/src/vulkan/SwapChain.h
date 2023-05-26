#pragma once
#include "Device.h"
#include "ImageView.h"
#include <vulkan/vulkan.hpp>

class SwapChain
{
public:
	SwapChain() = default;
	void Init(const Device& device, const Window& window, vk::SampleCountFlagBits sampleBits, bool vSync, bool hasDepth);
	void Create();
	void ReCreate();
	void AcquireNextImage(uint32_t* imageIndex, vk::Semaphore waitAcquireImage);
	void PresentImage(uint32_t imageIndex, vk::Semaphore waitDrawFinish);
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
	vk::Format m_Format = vk::Format::eB8G8R8A8Srgb;
	vk::ColorSpaceKHR m_ColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	vk::PresentModeKHR m_PresentMode = vk::PresentModeKHR::eFifo;
	vk::Extent2D m_Extent = { 0, 0 };
	uint32_t m_ImageCount = 0;	
	std::vector<vk::Image> m_Images;
	std::vector<vk::ImageView> m_ImageViews;
	vk::SampleCountFlagBits m_Samples;
	bool m_HasDepth;
};