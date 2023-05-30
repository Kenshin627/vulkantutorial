#pragma once
#include "CommandManager.h"
#include "Device.h"

#include <vulkan/vulkan.hpp>

struct Buffer
{
	void Create(Device& device, vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags, void* data);
	vk::Result Map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	void Unmap();
	void Bind(vk::DeviceSize offset = 0);
	void SetDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	void CopyFrom(void* src, size_t size);
	void CopyFrom(void* dst, void* src, size_t size);
	vk::Result Flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	vk::Result Invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	void Clear();
	static void CopyBuffer(vk::Buffer src, vk::DeviceSize srcOffset, vk::Buffer dst, vk::DeviceSize dstOffset, vk::DeviceSize size, vk::Queue queue, CommandManager& commandManager);
	uint64_t GetMemAddress() { return reinterpret_cast<uint64_t>(mapped); }
	Device m_Device;
	vk::Buffer m_Buffer = nullptr;
	vk::DeviceMemory m_Memory = nullptr;
	vk::DescriptorBufferInfo m_Descriptor;
	vk::BufferUsageFlags m_Usage;
	vk::DeviceSize m_Size = 0;;
	vk::DeviceSize m_Alignment = 0;
	vk::SharingMode m_SharingMode;
	vk::MemoryPropertyFlags m_MemoryPropertyFlags;
	void* mapped = nullptr;
};