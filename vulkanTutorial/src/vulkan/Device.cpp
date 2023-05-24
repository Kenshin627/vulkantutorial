#include "Device.h"
#include "../Core.h"

#include <set>
#include <string>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Device::Device(Window& window, vk::Instance vkInstance)
{
	CreateSurface(window, vkInstance);
	PickPhysicalDevice(vkInstance);
	CreateLogicDevice();
	CreateCommandPool();
}

Device::~Device() {}

void Device::CreateSurface(Window& window, vk::Instance vkInstance)
{
	vk::Win32SurfaceCreateInfoKHR surfaceInfo;
	surfaceInfo.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
	surfaceInfo.setHwnd(glfwGetWin32Window(window.GetNativeWindow()))
			   .setHinstance(GetModuleHandle(nullptr));
	VK_CHECK_RESULT(vkInstance.createWin32SurfaceKHR(&surfaceInfo, nullptr, &m_Surface));
}

void Device::PickPhysicalDevice(vk::Instance vkInstance)
{
	auto devices = vkInstance.enumeratePhysicalDevices();
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

vk::SampleCountFlags Device::CalcMaxSamplerCount(vk::PhysicalDeviceProperties properties)
{
	auto count = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
	if (count & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
	if (count & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
	if (count & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
	if (count & vk::SampleCountFlagBits::e8 ) { return vk::SampleCountFlagBits::e8;  }
	if (count & vk::SampleCountFlagBits::e4 ) { return vk::SampleCountFlagBits::e4;  }
	if (count & vk::SampleCountFlagBits::e2 ) { return vk::SampleCountFlagBits::e2;  }
	if (count & vk::SampleCountFlagBits::e1 ) { return vk::SampleCountFlagBits::e1;  }
}

uint32_t Device::FindMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags)
{
	uint32_t memoryCount = m_MemoryProperties.memoryTypeCount;
	for (uint32_t i = 0; i < memoryCount; i++)
	{
		if ((memoryTypeBits & (1 << i)) && ((m_MemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)) 
		{
			return i;
		}
	}
}

void Device::CreateCommandPool(uint32_t queueFamilyIndex)
{
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
	poolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queueFamilyIndex);
	VK_CHECK_RESULT(m_LogicDevice.createCommandPool(&poolInfo, nullptr, &m_DefaultCommandPool));
}

void Device::CreateCommandPool()
{
	CreateCommandPool(m_QueueFamilyIndices.GraphicQueueIndex.value());
}

vk::CommandBuffer Device::AllocateCommandBuffer(vk::CommandPool commandPool, vk::CommandBufferLevel level, bool begin)
{
	vk::CommandBuffer buffer;

	vk::CommandBufferAllocateInfo bufferAllocInfo;
	bufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	bufferAllocInfo.setCommandBufferCount(1)
				   .setCommandPool(commandPool)
				   .setLevel(level);
	VK_CHECK_RESULT(m_LogicDevice.allocateCommandBuffers(&bufferAllocInfo, &buffer));
	if (begin)
	{
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
		beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
				 .setPInheritanceInfo(nullptr)
				 .setPNext(nullptr);
		VK_CHECK_RESULT(buffer.begin(&beginInfo));
	}
	return buffer;
}

vk::CommandBuffer Device::AllocateCommandBuffer(vk::CommandBufferLevel level, bool begin)
{
	return AllocateCommandBuffer(m_DefaultCommandPool, level, begin);
}

void Device::FlushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, vk::CommandPool commandPool, bool free)
{
	commandBuffer.end();
	vk::SubmitInfo submitInfo;
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
		      .setPCommandBuffers(&commandBuffer);
	vk::Fence fence = CreateFence(vk::FenceCreateFlags());
	VK_CHECK_RESULT(queue.submit(1, &submitInfo, fence));
	VK_CHECK_RESULT(m_LogicDevice.waitForFences(1, &fence, true, UINT64_MAX));
	m_LogicDevice.destroyFence(fence, nullptr);
	if (free)
	{
		m_LogicDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
	}
}
void Device::FlushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free)
{
	FlushCommandBuffer(commandBuffer, queue, m_DefaultCommandPool, free);
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

void Device::CreateBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags, vk::Buffer* buffer, vk::DeviceMemory* memory, void* data)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	bufferInfo.setSharingMode(sharingMode)
			  .setSize(size)
			  .setUsage(usage);
	VK_CHECK_RESULT(m_LogicDevice.createBuffer(&bufferInfo, nullptr, buffer));

	vk::MemoryRequirements req = m_LogicDevice.getBufferMemoryRequirements(*buffer);
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(req.size)
			  .setMemoryTypeIndex(FindMemoryType(req.memoryTypeBits, memoryFlags));
	VK_CHECK_RESULT(m_LogicDevice.allocateMemory(&memoryInfo, nullptr, memory));

	if (data)
	{
		void* mapped;
		VK_CHECK_RESULT(m_LogicDevice.mapMemory(*memory, 0, size, {}, &mapped));
		memcpy(mapped, data, size);
		if ((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlagBits::eHostCoherent)
		{
			vk::MappedMemoryRange region;
			region.sType = vk::StructureType::eMappedMemoryRange;
			region.setMemory(*memory)
				  .setOffset(0)
				  .setSize(size);
			m_LogicDevice.flushMappedMemoryRanges(1, &region);
		}
		m_LogicDevice.unmapMemory(*memory);
	}
	m_LogicDevice.bindBufferMemory(*buffer, *memory, 0);
}

void Device::CreateBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags, Buffer* buffer, void* data)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	bufferInfo.setSharingMode(sharingMode)
		      .setSize(size)
		      .setUsage(usage);
	VK_CHECK_RESULT(m_LogicDevice.createBuffer(&bufferInfo, nullptr, &buffer->m_Buffer));

	vk::MemoryRequirements req = m_LogicDevice.getBufferMemoryRequirements(buffer->m_Buffer);
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(req.size)
			  .setMemoryTypeIndex(FindMemoryType(req.memoryTypeBits, memoryFlags));
	VK_CHECK_RESULT(m_LogicDevice.allocateMemory(&memoryInfo, nullptr, &buffer->m_Memory));
	buffer->SetDescriptor();
	buffer->m_Alignment = req.alignment;
	buffer->m_Device = m_LogicDevice;
	buffer->m_MemoryPropertyFlags = memoryFlags;
	buffer->m_SharingMode = sharingMode;
	buffer->m_Size = size;
	buffer->m_Usage = usage;
	if (data)
	{
		buffer->Map();
		buffer->CopyFrom(data, size);
		if ((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlagBits::eHostCoherent)
		{
			buffer->Flush();
		}
		buffer->Unmap();
	}
	buffer->Bind();
}

void Device::CopyBuffer(vk::Buffer src, vk::DeviceSize srcOffset, vk::Buffer dst, vk::DeviceSize dstOffset, vk::DeviceSize size, vk::Queue queue)
{
	auto command = AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

	vk::BufferCopy region;
	region.setDstOffset(dstOffset)
		  .setSrcOffset(srcOffset)
		  .setSize(size);
	command.copyBuffer(src, dst, 1, &region);

	FlushCommandBuffer(command, queue);
}