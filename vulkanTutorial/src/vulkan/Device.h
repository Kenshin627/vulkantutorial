#pragma once
#include "../Window.h"
#include <vector>
#include <optional>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

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
	uint32_t FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);
private:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicQueueIndex;
		std::optional<uint32_t> PresentQueueIndex;
		operator bool() { return GraphicQueueIndex.has_value() && PresentQueueIndex.has_value(); }
	};
private:
	void CreateSurface(Window& window, vk::Instance vkInstance);
	void PickPhysicalDevice(vk::Instance vkInstance);
	void CreateLogicDevice();
	bool QuerySwapchainASupport(const vk::PhysicalDevice& device);
	QueueFamilyIndices QueryQueueFamilyIndices(const vk::PhysicalDevice& device);
	vk::SampleCountFlags CalcMaxSamplerCount(vk::PhysicalDeviceProperties properties);
private:
	vk::PhysicalDevice m_PhysicalDevice;
	vk::Device m_LogicDevice;
	vk::SurfaceKHR m_Surface; 
	std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::Queue m_GraphicQueue;
	vk::Queue m_PresentQueue;
	vk::PhysicalDeviceMemoryProperties m_MemoryProperties;
	vk::SampleCountFlags m_MaxSamplerCount;
};