#include "../Core.h"
#include "Texture.h"
#include "stb_image.h"
#include "Buffer.h"

void Texture::Create(Device& device, const char* path, bool generateMipmaps)
{
	m_Device = device;
	int width, height, channel;
	stbi_set_flip_vertically_on_load(1);
	stbi_uc* pixels = stbi_load(path, &width, &height, &channel, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("read imagefile failed!");
	}
	vk::DeviceSize size = width * height * 4;
	m_MipLevel = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);
	if (!generateMipmaps)
	{
		m_MipLevel = 1;
	}

	Buffer stagingBuffer;
	stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pixels);

	m_Image.Create(m_Device, m_MipLevel, vk::SampleCountFlagBits::e1, vk::ImageType::e2D, vk::Extent3D(width, height, 1), vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
	m_Image.CreateImageView(vk::Format::eR8G8B8A8Srgb);

	m_Image.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);

	m_Image.CopyBufferToImage(stagingBuffer.m_Buffer, vk::Extent3D(width, height, 1), vk::ImageLayout::eShaderReadOnlyOptimal);

	if (generateMipmaps)
	{
		m_Image.GenerateMipMaps();
	}
	//stagingBuffer.Clear();
	CreateSampler();
	CreateDescriptor();
}

void Texture::Clear()
{
	m_Image.Clear();
}