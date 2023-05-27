#include "../Core.h"
#include "CubeMap.h"

#include "stb_image.h"
#include "Buffer.h"

void CubeMap::Create(Device& device, const std::vector<const char*>& paths)
{
	m_Device = device;
	Buffer stagingBuffer;
	vk::DeviceSize layerSize;
	vk::DeviceSize imageSize;
	stbi_uc* pixels = nullptr;
	uint64_t memAddress;
	for (uint32_t i = 0; i < paths.size(); i++)
	{
		pixels = stbi_load(paths[i], &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);
		if (pixels)
		{
			if (i == 0)
			{
				layerSize = m_Width * m_Height * 4;
				imageSize = layerSize * 6;
				stagingBuffer.Create(device, vk::BufferUsageFlagBits::eTransferSrc, imageSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, nullptr);
				stagingBuffer.Map();
				memAddress = stagingBuffer.GetMemAddress();
			}
			stagingBuffer.CopyFrom(reinterpret_cast<void*>(memAddress), pixels, static_cast<size_t>(layerSize));
			stbi_image_free(pixels);
			memAddress += layerSize;
		}
	}
	stagingBuffer.Unmap();

	m_Image.Create(device, 1, vk::SampleCountFlagBits::e1, vk::ImageType::e2D, vk::Extent3D(m_Width, m_Height, 1), vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 6, vk::ImageCreateFlagBits::eCubeCompatible);
	m_Image.CreateImageView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube);

	m_Image.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);
	m_Image.CopyBufferToImage(stagingBuffer.m_Buffer, vk::Extent3D(m_Width, m_Height, 1), vk::ImageLayout::eTransferDstOptimal);
	m_Image.TransiationLayout(vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);
	CreateSampler();
	CreateDescriptor();
}

void CubeMap::Clear()
{
	m_Image.Clear();
}

void CubeMap::CreateSampler()
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;
	samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(VK_FALSE)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eNever)
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipLodBias(0.0f)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMinLod(0.0f)
		.setMaxLod(0.0f)
		.setUnnormalizedCoordinates(VK_FALSE);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createSampler(&samplerInfo, nullptr, &m_Sampler));
}

void CubeMap::CreateDescriptor()
{
	m_Descriptor.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			    .setImageView(m_Image.GetVkImageView())
			    .setSampler(m_Sampler);
}

