#pragma once
#define VK_USE_PLATFORM_WIN32_KHR

#include "../Window.h"
#include "Buffer.h"

#include <vector>
#include <optional>

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicQueueIndex;
	std::optional<uint32_t> PresentQueueIndex;
	operator bool() { return GraphicQueueIndex.has_value() && PresentQueueIndex.has_value(); }
};

class Device
{
public:
	Device() = default;
	Device(Window& window, vk::Instance vkInstance);
	~Device();
	vk::Device GetLogicDevice() { return m_LogicDevice; }
	vk::PhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
	vk::SurfaceKHR GetSurface() { return m_Surface; }
	vk::Queue GetGraphicQueue() { return m_GraphicQueue; }
	vk::Queue GetPresentQueue() { return m_PresentQueue; }
	vk::CommandPool GetCommandPool() { return m_DefaultCommandPool; }
	uint32_t FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);
	bool QuerySwapchainASupport(const vk::PhysicalDevice& device);
	QueueFamilyIndices QueryQueueFamilyIndices(const vk::PhysicalDevice& device);
	vk::CommandBuffer AllocateCommandBuffer(vk::CommandPool commandPool, vk::CommandBufferLevel level, bool begin = true);
	vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level, bool begin = true);
	void FlushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, vk::CommandPool commandPool, bool free = true);
	void FlushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free = true);
	vk::Fence CreateFence(vk::FenceCreateFlags flags);
	void CreateBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags, vk::Buffer* buffer, vk::DeviceMemory* memory, void* data);
	void CreateBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags, Buffer* buffer, void* data);
	void CopyBuffer(vk::Buffer src, vk::DeviceSize srcOffset, vk::Buffer dst, vk::DeviceSize dstOffset, vk::DeviceSize size, vk::Queue queue);
private:
	void CreateSurface(Window& window, vk::Instance vkInstance);
	void PickPhysicalDevice(vk::Instance vkInstance);
	void CreateLogicDevice();
	void CreateCommandPool(uint32_t queueFamilyIndex);
	void CreateCommandPool();
	vk::SampleCountFlags CalcMaxSamplerCount(vk::PhysicalDeviceProperties properties);

private:
	vk::PhysicalDevice m_PhysicalDevice;
	vk::Device m_LogicDevice;
	vk::SurfaceKHR m_Surface; 
	std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	QueueFamilyIndices m_QueueFamilyIndices;
	vk::Queue m_GraphicQueue;
	vk::Queue m_PresentQueue;
	vk::PhysicalDeviceMemoryProperties m_MemoryProperties;
	vk::SampleCountFlags m_MaxSamplerCount;
	vk::CommandPool m_DefaultCommandPool;
};