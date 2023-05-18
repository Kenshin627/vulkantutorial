#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vector>
#include <optional>
#include <glm.hpp>

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

	struct Vertex
	{
		glm::vec2 Position;
		glm::vec3 Color;

		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription desc;
			desc.setBinding(0)
				.setStride(sizeof(Vertex))
				.setInputRate(vk::VertexInputRate::eVertex);
			return desc;
		}

		static std::vector<vk::VertexInputAttributeDescription> GetAttributes()
		{
			std::vector<vk::VertexInputAttributeDescription> result;
			vk::VertexInputAttributeDescription positionAttribute;
			positionAttribute.setBinding(0)
						     .setFormat(vk::Format::eR32G32Sfloat)
						     .setLocation(0)
						     .setOffset(offsetof(Vertex, Position));
			result.push_back(positionAttribute);

			vk::VertexInputAttributeDescription colorAttribute;
			colorAttribute.setBinding(0)
						  .setFormat(vk::Format::eR32G32B32Sfloat)
						  .setLocation(1)
						  .setOffset(offsetof(Vertex, Color));
			result.push_back(colorAttribute);
			return result;
		}
	};

	struct UniformBufferObject
	{
		glm::mat4 Proj;
		glm::mat4 View;
		glm::mat4 Model;
	};
private:
	void CreateInstance();
	void PickupPhysicalDevice();
	void CreateSurface();
	QueueFamilyIndices QueryQueueFmily(const vk::PhysicalDevice& device);
	void CreateLogicDevice();
	void CreateSwapchain();
	void CreateRenderPass();
	void CreateSetLayout();
	void CreatePipeLine();
	void CreateFrameBuffer();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffer();
	void RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
	void CreateAsyncObjects();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void UpdateDescriptorSet();
	void DrawFrame();
	vk::ShaderModule CompilerShader(const std::string& path);
	SwapchainProperties QuerySwapchainSupport(const vk::PhysicalDevice& device);
	uint32_t FindMemoryPropertyType(uint32_t memoryType, vk::MemoryPropertyFlags flags);
	void CreateBuffer(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize size, vk::BufferUsageFlags flags, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryPropertyFlags);
	vk::CommandBuffer OneTimeSubmitCommandBegin();
	void OneTimeSubmitCommandEnd(vk::CommandBuffer command);
	void UpdateUniformBuffers();

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

	std::vector<Vertex> m_VertexData = {
		{glm::vec2(-0.5, -0.5), glm::vec3(1.0, 0.0, 0.0)},
		{glm::vec2( 0.5, -0.5), glm::vec3(0.0, 1.0, 0.0)},
		{glm::vec2( 0.5,  0.5), glm::vec3(0.0, 1.0, 0.0)},
		{glm::vec2(-0.5,  0.5), glm::vec3(0.0, 0.0, 1.0)},
	};
	std::vector<uint16_t> m_Indices = {
		0, 1, 2,
		2, 3, 0
	};
	vk::Buffer m_VertexBuffer;
	vk::DeviceMemory m_VertexMemory;

	vk::Buffer m_IndexBuffer;
	vk::DeviceMemory m_IndexMemory;

	vk::Buffer m_UniformBuffer;
	vk::DeviceMemory m_UniformMemory;
	void* m_UniformMappedData;
	vk::DescriptorSetLayout m_SetLayout;
	vk::DescriptorPool m_DescriptorPool;
	vk::DescriptorSet m_DescriptorSet;
};