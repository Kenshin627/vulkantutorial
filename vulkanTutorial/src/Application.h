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
		vk::PresentModeKHR vk_Present;
		vk::Extent2D Extent;
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
	void CreateFrameBuffer();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
	void CreateAsyncObjects();
	void DrawFrame();
	vk::ShaderModule CompilerShader(const std::string& path);
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
	vk::PipelineLayout m_Layout;
	vk::Pipeline m_Pipeline;
	std::vector<vk::Framebuffer> m_FrameBuffers;
	vk::CommandPool m_CommandPool;
	vk::CommandBuffer m_CommandBuffer;
	vk::Fence m_InFlightFence;
	vk::Semaphore m_WaitAcquireImageSemaphore;
	vk::Semaphore m_WaitFinishDrawSemaphore;
	std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};