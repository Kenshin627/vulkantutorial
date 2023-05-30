#pragma once
#include "Device.h"
#include "ImageView.h"
#include "Image.h"
#include "FrameBuffer.h"
#include <vulkan/vulkan.hpp>

class Application;
class SwapChain
{
public:
	void Init(Device& device, const Window& window, vk::SampleCountFlagBits sampleBits, bool vSync, bool hasDepth);
	void Create();
	void ReCreate();
	void AcquireNextImage(uint32_t* imageIndex, vk::Semaphore waitAcquireImage, Application* app);
	void PresentImage(uint32_t imageIndex, vk::Semaphore waitDrawFinish, Application* app);
	~SwapChain();
	void Clear();
	vk::SwapchainKHR GetSwapChain() { return m_SwapChain; }
	vk::Format GetFormat() { return m_Format; }
	vk::ColorSpaceKHR GetColorSpace() { return m_ColorSpace; }
	vk::Extent2D GetExtent() { return m_Extent; }
	std::vector<vk::Image> GetSwapChainImages() { return m_SwapChainImages; }
	std::vector<ImageView> GetSwapChainImageViews() { return m_SwapChainImageViews; }
	vk::RenderPass GetRenderPass() { return m_RenderPass; }
	std::vector<FrameBuffer>& GetFrameBuffers() { return m_FrameBuffers; }
	uint32_t GetImageCount() { return m_ImageCount; }
private:
	void CreateColorAttachment();
	void CreateDepthStencilAttachment();
	void CreateFrameBuffers();
	void CreateRenderPass();
private:
	Device m_Device;
	CommandManager m_CommandManager;
	bool m_vSync;
	Window m_Window;
	vk::SwapchainKHR m_SwapChain;
	vk::SurfaceCapabilitiesKHR m_Capabilities;
	vk::Format m_Format = vk::Format::eB8G8R8A8Srgb;
	vk::ColorSpaceKHR m_ColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	vk::PresentModeKHR m_PresentMode = vk::PresentModeKHR::eFifo;
	vk::Extent2D m_Extent = { 0, 0 };
	uint32_t m_ImageCount = 0;	
	std::vector<vk::Image> m_SwapChainImages;
	std::vector<ImageView> m_SwapChainImageViews;
	vk::SampleCountFlagBits m_Samples;
	bool m_HasDepth;
	std::vector<Image> m_ColorAttachments;
	std::vector<Image> m_DepthAttachments;
	std::vector<FrameBuffer> m_FrameBuffers;
	vk::RenderPass m_RenderPass;
};