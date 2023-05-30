#include "../Core.h"
#include "Device.h"
#include <set>
#include <string>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Device::Device(Window& window)
{
	CreateInstance();
	CreateSurface(window);
	PickPhysicalDevice();
	CreateLogicDevice();
	m_CommandManager.SetContext(m_LogicDevice, QueryQueueFamilyIndices(m_PhysicalDevice).GraphicQueueIndex.value());
}

Device::~Device() {}

void Device::CreateInstance()
{
	vk::ApplicationInfo appInfo;
	appInfo.sType = vk::StructureType::eApplicationInfo;
	appInfo.setApiVersion(VK_API_VERSION_1_3);

	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	vk::InstanceCreateInfo instanceInfo;
	instanceInfo.sType = vk::StructureType::eInstanceCreateInfo;
	instanceInfo.setPApplicationInfo(&appInfo)
				.setEnabledExtensionCount(extensionCount)
				.setPpEnabledExtensionNames(extensions)
				.setEnabledLayerCount(0)
				.setPpEnabledLayerNames(nullptr);
	if (vk::createInstance(&instanceInfo, nullptr, &m_VkInstance) != vk::Result::eSuccess)
	{
		throw std::runtime_error("vk instance create failed!");
	}
}

void Device::CreateSurface(Window& window)
{
	vk::Win32SurfaceCreateInfoKHR surfaceInfo;
	surfaceInfo.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
	surfaceInfo.setHwnd(glfwGetWin32Window(window.GetNativeWindow()))
			   .setHinstance(GetModuleHandle(nullptr));
	VK_CHECK_RESULT(m_VkInstance.createWin32SurfaceKHR(&surfaceInfo, nullptr, &m_Surface));
}

void Device::PickPhysicalDevice()
{
	auto devices = m_VkInstance.enumeratePhysicalDevices();
	uint32_t index = 1;
	for (auto& device : devices)
	{
		std::cout << index++ << ":" << device.getProperties().deviceName << std::endl;
		auto property = device.getProperties();
		auto feature = device.getFeatures();
		auto extensions = device.enumerateDeviceExtensionProperties();

		std::set<std::string> supportExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
		for (auto& extension : extensions)
		{
			supportExtensions.erase(extension.extensionName);
		}
		auto queueFamilyIndices = QueryQueueFamilyIndices(device);
		if (property.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && feature.geometryShader && supportExtensions.empty() && QuerySwapchainASupport(device) && queueFamilyIndices)
		{
			m_PhysicalDevice = device;
			m_QueueFamilyIndices = queueFamilyIndices;
			m_MemoryProperties = device.getMemoryProperties();
			m_MaxSamplerCount = CalcMaxSamplerCount(property);

			m_SurfaceFormats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
			m_SurfacePresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);
			m_SurfaceCapability = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
		}
	}
}

bool Device::QuerySwapchainASupport(const vk::PhysicalDevice& device)
{
	auto surfaceFormats = device.getSurfaceFormatsKHR(m_Surface);
	auto surfacePresentModes = device.getSurfacePresentModesKHR(m_Surface);
	return !surfaceFormats.empty() && !surfacePresentModes.empty();
}

QueueFamilyIndices Device::QueryQueueFamilyIndices(const vk::PhysicalDevice& device)
{
	QueueFamilyIndices indices;
	auto queueFamilies = device.getQueueFamilyProperties();
	uint32_t index = 0;
	for (const auto& queue : queueFamilies)
	{
		if ((queue.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics)
		{
			indices.GraphicQueueIndex = index;
		}
		vk::Bool32 presentSupported;
		auto surfaceSupportRes = device.getSurfaceSupportKHR(index, m_Surface, &presentSupported);
		if (presentSupported)
		{
			indices.PresentQueueIndex = index;
		}
		if (indices)
		{
			break;
		}
		index++;
	}
	return indices;
}

void Device::CreateLogicDevice()
{
	vk::PhysicalDeviceFeatures feature;
	feature.setSampleRateShading(true);

	float priority = 1.0f;
	auto queueFamilyIndices = QueryQueueFamilyIndices(m_PhysicalDevice);
	std::array<uint32_t, 2> queueIndices = { queueFamilyIndices.GraphicQueueIndex.value(), queueFamilyIndices.PresentQueueIndex.value() };
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (const uint32_t& index : queueIndices)
	{
		vk::DeviceQueueCreateInfo queueInfo;
		queueInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
		queueInfo.setPQueuePriorities(&priority)
				 .setQueueCount(1)
				 .setQueueFamilyIndex(index);
		queueCreateInfos.push_back(queueInfo);
	}

	vk::DeviceCreateInfo deviceInfo;
	deviceInfo.sType = vk::StructureType::eDeviceCreateInfo;
	deviceInfo.setEnabledExtensionCount(static_cast<uint32_t>(m_DeviceExtensions.size()))
			  .setPpEnabledExtensionNames(m_DeviceExtensions.data())
			  .setPEnabledFeatures(&feature)
			  .setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
			  .setPQueueCreateInfos(queueCreateInfos.data());
	VK_CHECK_RESULT(m_PhysicalDevice.createDevice(&deviceInfo, nullptr, &m_LogicDevice));
	m_GraphicQueue = m_LogicDevice.getQueue(queueFamilyIndices.GraphicQueueIndex.value(), 0);
	m_PresentQueue = m_LogicDevice.getQueue(queueFamilyIndices.PresentQueueIndex.value(), 0);
}

vk::SampleCountFlagBits Device::CalcMaxSamplerCount(vk::PhysicalDeviceProperties properties)
{
	auto count = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
	if (count & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
	if (count & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
	if (count & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
	if (count & vk::SampleCountFlagBits::e8 ) { return vk::SampleCountFlagBits::e8;  }
	if (count & vk::SampleCountFlagBits::e4 ) { return vk::SampleCountFlagBits::e4;  }
	if (count & vk::SampleCountFlagBits::e2 ) { return vk::SampleCountFlagBits::e2;  }
	if (count & vk::SampleCountFlagBits::e1 ) { return vk::SampleCountFlagBits::e1;  }
	return vk::SampleCountFlagBits::e1;
}

vk::Fence Device::CreateFence(vk::FenceCreateFlags flags)
{
	vk::Fence fence;
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.setFlags(flags);
	VK_CHECK_RESULT(m_LogicDevice.createFence(&fenceInfo, nullptr, &fence));
	return fence;
}

uint32_t Device::FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags)
{
	vk::PhysicalDeviceMemoryProperties memoryProperties;
	m_PhysicalDevice.getMemoryProperties(&memoryProperties);
	uint32_t memoryCount = memoryProperties.memoryTypeCount;
	for (uint32_t i = 0; i < memoryCount; i++)
	{
		if ((memoryTypeBits & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags))
		{
			return i;
		}
	}
	throw std::runtime_error("can not found suitable memoryType!");
}

vk::Format Device::FindImageFormatDeviceSupport(const std::vector<vk::Format> formats, vk::ImageTiling tiling, vk::FormatFeatureFlags featureFlags)
{
	for (auto& format : formats)
	{
		auto formatProperties = m_PhysicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && ((formatProperties.linearTilingFeatures & featureFlags) == featureFlags))
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && ((formatProperties.optimalTilingFeatures & featureFlags) == featureFlags))
		{
			return format;
		}
	}
	throw std::runtime_error("image Format not found!");
}

bool Device::HasStencil(vk::Format format)
{
	if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
	{
		return true;
	}
	return false;
}