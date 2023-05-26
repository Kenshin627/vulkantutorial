#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include "../Window.h"
#include "CommandManager.h"
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
	Device(Window& window);
	~Device();
	vk::Device GetLogicDevice() { return m_LogicDevice; }
	vk::PhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
	vk::SurfaceKHR GetSurface() { return m_Surface; }
	vk::Queue GetGraphicQueue() { return m_GraphicQueue; }
	vk::Queue GetPresentQueue() { return m_PresentQueue; }
	std::vector<vk::SurfaceFormatKHR> GetSurfaceSupportFormats() { return m_SurfaceFormats; }
	std::vector<vk::PresentModeKHR> GetSurfaceSupportPresentModes() { return m_SurfacePresentModes; }
	vk::SurfaceCapabilitiesKHR GetSurfaceSupportCapability() { return m_SurfaceCapability; }
	vk::SampleCountFlagBits GetMaxSampleCount() { return m_MaxSamplerCount; }
	CommandManager& GetCommandManager() { return m_CommandManager; }
	uint32_t FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);
	bool QuerySwapchainASupport(const vk::PhysicalDevice& device);
	QueueFamilyIndices QueryQueueFamilyIndices(const vk::PhysicalDevice& device);
	vk::Format FindImageFormatDeviceSupport(const std::vector<vk::Format> formats, vk::ImageTiling tiling, vk::FormatFeatureFlags featureFlags);
	bool HasStencil(vk::Format format);
private:
	void CreateInstance();
	vk::Fence CreateFence(vk::FenceCreateFlags flags);
	void CreateSurface(Window& window);
	void PickPhysicalDevice();
	void CreateLogicDevice();
	vk::SampleCountFlagBits CalcMaxSamplerCount(vk::PhysicalDeviceProperties properties);

private:
	vk::Instance m_VkInstance;
	vk::PhysicalDevice m_PhysicalDevice;
	vk::Device m_LogicDevice;
	vk::SurfaceKHR m_Surface; 
	CommandManager m_CommandManager;
	std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	QueueFamilyIndices m_QueueFamilyIndices;
	vk::Queue m_GraphicQueue;
	vk::Queue m_PresentQueue;
	vk::PhysicalDeviceMemoryProperties m_MemoryProperties;
	vk::SampleCountFlagBits m_MaxSamplerCount;
	std::vector<vk::SurfaceFormatKHR> m_SurfaceFormats;
	std::vector<vk::PresentModeKHR> m_SurfacePresentModes;
	vk::SurfaceCapabilitiesKHR m_SurfaceCapability;
};