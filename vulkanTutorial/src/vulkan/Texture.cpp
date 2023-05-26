#include "../Core.h"
#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Buffer.h"

void Texture::Create(Device& device, CommandManager& commandManager, const char* path, bool generateMipmaps)
{
	m_Device = device;
	m_CommandManager = commandManager;
	int width, height, channel;
	stbi_uc* pixels = stbi_load(path, &width, &height, &channel, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("read imagefile failed!");
	}
	vk::DeviceSize size = width * height * 4;
	m_MipLevel = std::floor(std::log2(std::max(width, height))) + 1;
	if (!generateMipmaps)
	{
		m_MipLevel = 1;
	}

	Buffer stagingBuffer;
	stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pixels);

	m_Image.Create(m_Device, commandManager, m_MipLevel, vk::SampleCountFlagBits::e1, vk::ImageType::e2D, vk::Extent3D(width, height, 1), vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive);
	m_Image.CreateImageView(vk::Format::eR8G8B8A8Srgb);

	m_Image.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);

	m_Image.CopyBufferToImage(stagingBuffer.m_Buffer, vk::Extent3D(width, height, 1), vk::ImageLayout::eShaderReadOnlyOptimal);

	if (generateMipmaps)
	{
		m_Image.GenerateMipMaps();
	}
	//stagingBuffer.Clear();
	CreateSampler();
}

void Texture::Clear()
{
	m_Image.Clear();
}

void Texture::CreateSampler()
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;
	samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(VK_FALSE)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipLodBias(0.0f)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMinLod(0.0f)
		.setMaxLod(static_cast<float>(m_MipLevel))
		.setUnnormalizedCoordinates(VK_FALSE);
	if (m_Device.GetLogicDevice().createSampler(&samplerInfo, nullptr, &m_Sampler) != vk::Result::eSuccess)
	{
		throw std::runtime_error("sampler create failed!");
	}
}