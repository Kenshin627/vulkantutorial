#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vector>
#include <optional>

class Application
{
public:
	Application() = default;
	~Application() = default;
	void Run();
	void InitWindow();
	void InitVulkan();
	void RenderLoop();
	void Clear();
private:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicFamily;
		std::optional<uint32_t> PresentFamily;
		operator bool () { return GraphicFamily.has_value() && PresentFamily.has_value(); }
	};

	struct SwapchainProperties
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presents;
	};

	struct SwapChain
	{
		vk::SwapchainKHR vk_SwapChain;
		vk::SurfaceCapabilitiesKHR vk_Capabilities;
		vk::SurfaceFormatKHR vk_Format;
		vk::PresentModeKHR vk__Present;
		std::vector<vk::Image> Images;
		std::vector<vk::ImageView> ImageViews;
	};
private:
	void CreateInstance();
	void PickupPhysicalDevice();
	void CreateSurface();
	QueueFamilyIndices QueryQueueFmily(const vk::PhysicalDevice& device);
	void CreateLogicDevice();
	void CreateSwapchain();
	void CreateRenderPass();
	void CreatePipeLine();
	SwapchainProperties QuerySwapchainSupport(const vk::PhysicalDevice& device);

private:
	GLFWwindow* m_Window;
	vk::Instance m_VKInstance;
	vk::PhysicalDevice m_PhysicalDevice;
	vk::SurfaceKHR m_Surface;
	vk::Device m_Device;
	vk::Queue m_GraphicQueue;
	vk::Queue m_PresentQueue;

	SwapChain m_SwapChain;
	vk::RenderPass m_RenderPass;
	std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};