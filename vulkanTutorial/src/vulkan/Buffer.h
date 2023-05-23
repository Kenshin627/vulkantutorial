#pragma once
#include "Device.h"
#include <vulkan/vulkan.hpp>

class Buffer
{
public:
	Buffer(Device device, vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags);
	~Buffer();
private:
	Device m_Device;
	vk::Buffer m_Buffer;
	vk::DeviceMemory m_Memory;
	vk::BufferUsageFlags m_Usage;
	vk::DeviceSize m_Size;
	vk::SharingMode m_SharingMode;
	vk::MemoryPropertyFlags m_MemoryPropertyFlags;
};