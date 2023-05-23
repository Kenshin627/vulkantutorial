#pragma once
#include <vulkan/vulkan.hpp>

struct Buffer
{

	Buffer() = default;
	~Buffer();
	vk::Result Map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	void Unmap();
	void Bind(vk::DeviceSize offset = 0);
	void SetDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	void CopyFrom(void* data, vk::DeviceSize size);
	vk::Result Flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	vk::Result Invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
	void Destroy();

	vk::Device m_Device = nullptr;
	vk::Buffer m_Buffer = nullptr;
	vk::DeviceMemory m_Memory;
	vk::DescriptorBufferInfo m_Descriptor;
	vk::BufferUsageFlags m_Usage;
	vk::DeviceSize m_Size = 0;;
	vk::DeviceSize m_Alignment = 0;
	vk::SharingMode m_SharingMode;
	vk::MemoryPropertyFlags m_MemoryPropertyFlags;
	void* mapped = nullptr;
};