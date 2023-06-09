#include "../Core.h"
#include "Image.h"

void Image::Create(Device& device, uint32_t mipLevels, vk::SampleCountFlagBits samplerCount, vk::ImageType type, vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vk::MemoryPropertyFlags memoryFlags, vk::ImageLayout initialLayout, vk::SharingMode sharingMode, uint32_t arrayLayers, vk::ImageCreateFlags flag)
{
	m_Device = device;
	m_Size = size;
	m_Format = format;
	m_MipLevel = mipLevels;
	m_Layers = arrayLayers;
	vk::Device vkDevice = device.GetLogicDevice();
	vk::ImageCreateInfo imageInfo;
	imageInfo.sType = vk::StructureType::eImageCreateInfo;
	imageInfo.setArrayLayers(arrayLayers)
		     .setExtent(size)
		     .setFormat(format)
		     .setImageType(type)
		     .setInitialLayout(initialLayout)
		     .setMipLevels(mipLevels)
		     .setSamples(samplerCount)
		     .setSharingMode(sharingMode)
		     .setTiling(tiling)
		     .setUsage(usage)
			 .setFlags(flag);
	VK_CHECK_RESULT(vkDevice.createImage(&imageInfo, nullptr, &m_VkImage));

	vk::MemoryRequirements requirement = vkDevice.getImageMemoryRequirements(m_VkImage);
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(requirement.size)
		      .setMemoryTypeIndex(m_Device.FindMemoryType(requirement.memoryTypeBits, memoryFlags));
	VK_CHECK_RESULT(vkDevice.allocateMemory(&memoryInfo, nullptr, &m_Memory));
	vkDevice.bindImageMemory(m_VkImage, m_Memory, 0);
}

void Image::TransiationLayout(vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess, vk::ImageLayout srcLayout, vk::PipelineStageFlags dstStage, vk::AccessFlags dstAccess, vk::ImageLayout dstLayout, vk::ImageAspectFlags aspectFlags)
{
	auto command = m_Device.GetCommandManager().AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);
		vk::ImageSubresourceRange region;
		region.setAspectMask(aspectFlags)
			  .setBaseArrayLayer(0)
			  .setBaseMipLevel(0)
			  .setLayerCount(m_Layers)
			  .setLevelCount(m_MipLevel);
		vk::ImageMemoryBarrier barrier;
		barrier.sType = vk::StructureType::eImageMemoryBarrier;
		barrier.setImage(m_VkImage)
			   .setSubresourceRange(region)
			   .setSrcAccessMask(srcAccess)
			   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			   .setOldLayout(srcLayout)
			   .setDstAccessMask(dstAccess)
			   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			   .setNewLayout(dstLayout);
		command.pipelineBarrier(srcStage, dstStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);
		m_Device.GetCommandManager().FlushCommandBuffer(command, m_Device.GetGraphicQueue());
}

void Image::CopyBufferToImage(vk::Buffer srcBuffer, vk::Extent3D size, vk::ImageLayout layout)
{
	auto command = m_Device.GetCommandManager().AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

	vk::ImageSubresourceLayers layer;
	layer.setAspectMask(vk::ImageAspectFlagBits::eColor)
		 .setBaseArrayLayer(0)
		 .setLayerCount(m_Layers)
		 .setMipLevel(0);
	vk::BufferImageCopy region;
	region.setBufferImageHeight(0)
		  .setBufferOffset(0)
		  .setBufferRowLength(0)
		  .setImageExtent(size)
		  .setImageOffset(0)
		  .setImageSubresource(layer);
	command.copyBufferToImage(srcBuffer, m_VkImage, layout, 1, &region);

	m_Device.GetCommandManager().FlushCommandBuffer(command, m_Device.GetGraphicQueue());
}

void Image::GenerateMipMaps()
{
	if (m_MipLevel <= 1)
	{
		return;
	}
	uint32_t mipWidth = m_Size.width;
	uint32_t mipHeight = m_Size.height;
	auto command = m_Device.GetCommandManager().AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

	vk::ImageSubresourceRange region;
	region.setAspectMask(vk::ImageAspectFlagBits::eColor)
		  .setBaseArrayLayer(0)
		  .setLayerCount(1)
		  .setLevelCount(1);

	vk::ImageMemoryBarrier barrier;
	barrier.sType = vk::StructureType::eImageMemoryBarrier;
	barrier.setImage(m_VkImage)
		   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

	for (uint32_t i = 1; i < m_MipLevel; i++)
	{
		region.setBaseMipLevel(i - 1);
		barrier.setSubresourceRange(region)
			   .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			   .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
			   .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			   .setDstAccessMask(vk::AccessFlagBits::eTransferRead);
		command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0, nullptr, 1, &barrier);

		vk::ImageBlit blit;
		std::array<vk::Offset3D, 2> dstOffsets = { vk::Offset3D(0, 0, 0), vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1) };
		std::array<vk::Offset3D, 2> srcOffsets = { vk::Offset3D(0, 0, 0), vk::Offset3D(mipWidth, mipHeight, 1) };
		vk::ImageSubresourceLayers srcLayers;
		srcLayers.setAspectMask(vk::ImageAspectFlagBits::eColor)
			     .setBaseArrayLayer(0)
			     .setLayerCount(1)
			     .setMipLevel(i - 1);

		vk::ImageSubresourceLayers dstLayers;
		dstLayers.setAspectMask(vk::ImageAspectFlagBits::eColor)
			     .setBaseArrayLayer(0)
			     .setLayerCount(1)
			     .setMipLevel(i);

		blit.setDstOffsets(dstOffsets)
			.setSrcOffsets(srcOffsets)
			.setSrcSubresource(srcLayers)
			.setDstSubresource(dstLayers);

		command.blitImage(m_VkImage, vk::ImageLayout::eTransferSrcOptimal, m_VkImage, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
			   .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
			   .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			   .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);
		mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
		mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
	}

	region.setBaseMipLevel(m_MipLevel - 1);
	barrier.setSubresourceRange(region)
		   .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
		   .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
		   .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		   .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);

	m_Device.GetCommandManager().FlushCommandBuffer(command, m_Device.GetGraphicQueue());
}

void Image::CreateImageView(vk::Format format, vk::ImageAspectFlags aspectFlag, vk::ImageViewType viewType, vk::ComponentMapping mapping)
{
	vk::ImageSubresourceRange region;
	region.setAspectMask(aspectFlag)
		  .setBaseArrayLayer(0)
		  .setBaseMipLevel(0)
		  .setLayerCount(m_Layers)
		  .setLevelCount(m_MipLevel);
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	viewInfo.setComponents(mapping)
		    .setFormat(format)
		    .setImage(m_VkImage)
		    .setViewType(viewType)
		    .setSubresourceRange(region);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createImageView(&viewInfo, nullptr, &m_View));
}

void Image::Clear()
{
	vk::Device vkDevice = m_Device.GetLogicDevice();
	vkDevice.destroyImageView(m_View, nullptr);
	vkDevice.destroyImage(m_VkImage, nullptr);
	vkDevice.freeMemory(m_Memory, nullptr);
}

void Image::CreateSampler()
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
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createSampler(&samplerInfo, nullptr, &m_Sampler));
}

void Image::CreateDescriptor()
{
	m_Descriptor.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(m_View)
				.setSampler(m_Sampler);
}

void Image::QueryImageType()
{
	//TODO::temp
	switch (m_Format)
	{
	case vk::Format::eD32Sfloat:	   m_ImageType = ImageType::Depth; break;
	case vk::Format::eD32SfloatS8Uint: m_ImageType = ImageType::Depth; break;
	case vk::Format::eD24UnormS8Uint:  m_ImageType = ImageType::Depth; break;
		m_ImageType = ImageType::Color;
	}
}