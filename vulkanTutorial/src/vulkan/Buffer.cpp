#include "../Core.h"
#include "Buffer.h"

void Buffer::Create(Device& device, vk::BufferUsageFlags usage, vk::DeviceSize size, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryFlags, void* data)
{
	m_Device = device;
	m_Usage = usage;
	m_Size = size;
	m_SharingMode = sharingMode;
	m_MemoryPropertyFlags = memoryFlags;

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	bufferInfo.setSharingMode(sharingMode)
		      .setSize(size)
		      .setUsage(usage);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createBuffer(&bufferInfo, nullptr, &m_Buffer));
	vk::MemoryRequirements requirement = m_Device.GetLogicDevice().getBufferMemoryRequirements(m_Buffer);
	m_Alignment = requirement.alignment;
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(requirement.size)
			  .setMemoryTypeIndex(m_Device.FindMemoryType(requirement.memoryTypeBits, memoryFlags));
	VK_CHECK_RESULT(m_Device.GetLogicDevice().allocateMemory(&memoryInfo, nullptr, &m_Memory));
	SetDescriptor();
	if (data)
	{
		Map();
		CopyFrom(data, size);
		if((memoryFlags & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlagBits::eHostCoherent)
		{
			Flush();
		}
		Unmap();
	}
	Bind();
}

vk::Result Buffer::Map(vk::DeviceSize size, vk::DeviceSize offset)
{
	return m_Device.GetLogicDevice().mapMemory(m_Memory, offset, size, {}, &mapped);
}

void Buffer::Unmap()
{
	if (mapped)
	{
		m_Device.GetLogicDevice().unmapMemory(m_Memory);
		mapped = nullptr;
	}
}

void Buffer::Bind(vk::DeviceSize offset)
{
	m_Device.GetLogicDevice().bindBufferMemory(m_Buffer, m_Memory, offset);
}

void Buffer::SetDescriptor(vk::DeviceSize size, vk::DeviceSize offset)
{
	m_Descriptor.setBuffer(m_Buffer)
			    .setOffset(offset)
			    .setRange(size);
}

void Buffer::CopyFrom(void* data, vk::DeviceSize size)
{
	memcpy(mapped, data, size);
}

vk::Result Buffer::Flush(vk::DeviceSize size, vk::DeviceSize offset)
{
	vk::MappedMemoryRange region;
	region.sType = vk::StructureType::eMappedMemoryRange;
	region.setMemory(m_Memory)
		  .setOffset(offset)
		  .setSize(size);
	return m_Device.GetLogicDevice().flushMappedMemoryRanges(1, &region);
}

vk::Result Buffer::Invalidate(vk::DeviceSize size, vk::DeviceSize offset)
{
	vk::MappedMemoryRange region;
	region.sType = vk::StructureType::eMappedMemoryRange;
	region.setMemory(m_Memory)
		  .setSize(size)
		  .setOffset(offset);
	return m_Device.GetLogicDevice().invalidateMappedMemoryRanges(1, &region);
}

void Buffer::Clear()
{
	if (m_Buffer)
	{
		m_Device.GetLogicDevice().destroyBuffer(m_Buffer);
	}

	if (m_Memory)
	{
		m_Device.GetLogicDevice().freeMemory(m_Memory);
	}
}

void Buffer::CopyBuffer(vk::Buffer src, vk::DeviceSize srcOffset, vk::Buffer dst, vk::DeviceSize dstOffset, vk::DeviceSize size, vk::Queue queue, CommandManager& commandManager)
{
	auto command = commandManager.AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

	vk::BufferCopy region;
	region.setDstOffset(dstOffset)
		  .setSrcOffset(srcOffset)
		  .setSize(size);
	command.copyBuffer(src, dst, 1, &region);

	commandManager.FlushCommandBuffer(command, queue);
}