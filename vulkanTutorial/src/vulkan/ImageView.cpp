#include "../Core.h"
#include "ImageView.h"

void ImageView::Create(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlag, uint32_t mipLevels, vk::ImageViewType viewType, vk::ComponentMapping mapping)
{
	vk::ImageSubresourceRange region;
	region.setAspectMask(aspectFlag)
		  .setBaseArrayLayer(0)
		  .setBaseMipLevel(0)
		  .setLayerCount(1)
		  .setLevelCount(mipLevels);
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	viewInfo.setComponents(mapping)
		    .setFormat(format)
		    .setImage(image)
		    .setSubresourceRange(region)
		    .setViewType(viewType);
	VK_CHECK_RESULT(device.createImageView(&viewInfo, nullptr, &m_View));
	m_Device = device;
	m_Image = image;
}

void ImageView::Clear()
{
	if (m_View)
	{
		m_Device.destroyImageView(m_View, nullptr);
	}
}