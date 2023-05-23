#include "../Core.h"
#include "Buffer.h"

Buffer::Buffer(Device device, vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags)
{
	m_Device = device;
	auto vkDevice = m_Device.GetLogicDevice();
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	bufferInfo.setSharingMode(sharingMode)
			  .setSize(size)
			  .setUsage(usage);
	VK_CHECK_RESULT(vkDevice.createBuffer(&bufferInfo, nullptr, &m_Buffer));

	vk::MemoryRequirements req = vkDevice.getBufferMemoryRequirements(m_Buffer);
	
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(req.size)
			  .setMemoryTypeIndex(m_Device.FindMemoryType(req.memoryTypeBits, memoryFlags));
	VK_CHECK_RESULT(vkDevice.allocateMemory(&memoryInfo, nullptr, &m_Memory));

	vkDevice.bindBufferMemory(m_Buffer, m_Memory, 0);
}

Buffer::~Buffer()
{
	
}