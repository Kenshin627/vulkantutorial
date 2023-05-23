#include "../Core.h"
#include "Buffer.h"

Buffer::~Buffer()
{
	Destroy();
}

vk::Result Buffer::Map(vk::DeviceSize size, vk::DeviceSize offset)
{
	return m_Device.mapMemory(m_Memory, offset, size, {}, &mapped);
}

void Buffer::Unmap()
{
	if (mapped)
	{
		m_Device.unmapMemory(m_Memory);
		mapped = nullptr;
	}
}

void Buffer::Bind(vk::DeviceSize offset)
{
	m_Device.bindBufferMemory(m_Buffer, m_Memory, offset);
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
	return m_Device.flushMappedMemoryRanges(1, &region);
}

vk::Result Buffer::Invalidate(vk::DeviceSize size, vk::DeviceSize offset)
{
	vk::MappedMemoryRange region;
	region.sType = vk::StructureType::eMappedMemoryRange;
	region.setMemory(m_Memory)
		  .setSize(size)
		  .setOffset(offset);
	return m_Device.invalidateMappedMemoryRanges(1, &region);
}

void Buffer::Destroy()
{
	if (m_Buffer)
	{
		m_Device.destroyBuffer(m_Buffer);
	}

	if (m_Memory)
	{
		m_Device.freeMemory(m_Memory);
	}
}