#include "../Core.h"
#include "SwapChain.h"

void SwapChain::Init(const Device& device, const Window& window, vk::SampleCountFlagBits sampleBits, bool vSync, bool hasDepth)
{
	m_Device = device;
	m_Window = window;
	m_vSync = vSync;
	m_Samples = sampleBits;
	m_HasDepth = hasDepth;
	Create();
}

void SwapChain::Create()
{
	auto formats = m_Device.GetSurfaceSupportFormats();
	auto presentModes = m_Device.GetSurfaceSupportPresentModes();
	auto capability = m_Device.GetSurfaceSupportCapability();

	vk::SurfaceFormatKHR surfaceFormat = formats[0];
	for (auto& fm : formats)
	{
		if (fm.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && fm.format == vk::Format::eB8G8R8A8Srgb)
		{
			surfaceFormat = fm;
			break;
		}
	}
	m_Format = surfaceFormat.format;
	m_ColorSpace = surfaceFormat.colorSpace;

	m_PresentMode = vk::PresentModeKHR::eFifo;
	if (!m_vSync)
	{
		for (const auto& mode : presentModes)
		{
			if (mode == vk::PresentModeKHR::eMailbox)
			{
				m_PresentMode = mode;
				break;
			}
			if (mode == vk::PresentModeKHR::eImmediate)
			{
				m_PresentMode = mode;
			}
		}
	}

	if (capability.currentExtent.width != UINT32_MAX)
	{
		m_Extent = capability.currentExtent;
	}
	else
	{
		int width, height;
		m_Window.GetFrameBufferSize(&width, &height);
		width = std::clamp(static_cast<uint32_t>(width), capability.minImageExtent.width, capability.maxImageExtent.width);
		height = std::clamp(static_cast<uint32_t>(height), capability.minImageExtent.height, capability.maxImageExtent.height);
		m_Extent.setWidth(width).setHeight(height);
	}

	m_ImageCount = capability.minImageCount + 1;
	if (capability.maxImageCount > 0 && m_ImageCount > capability.maxImageCount)
	{
		m_ImageCount = capability.maxImageCount;
	}
	vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment;
	if (capability.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc)
	{
		usage |= vk::ImageUsageFlagBits::eTransferSrc;
	}
	if (capability.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)
	{
		usage |= vk::ImageUsageFlagBits::eTransferDst;
	}
	vk::SurfaceTransformFlagBitsKHR preTransform;
	if (capability.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
	{
		preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	}
	else
	{
		preTransform = capability.currentTransform;
	}
	vk::SharingMode sharingMode;
	auto queueIndices = m_Device.QueryQueueFamilyIndices(m_Device.GetPhysicalDevice());
	if (queueIndices.GraphicQueueIndex == queueIndices.PresentQueueIndex)
	{
		sharingMode = vk::SharingMode::eExclusive;
	}
	else
	{
		sharingMode = vk::SharingMode::eConcurrent;
	}
	vk::SwapchainCreateInfoKHR swapChainInfo;
	swapChainInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
	swapChainInfo.setClipped(true)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setImageArrayLayers(1)
		.setImageColorSpace(m_ColorSpace)
		.setImageExtent(m_Extent)
		.setImageFormat(m_Format)
		.setImageSharingMode(sharingMode)
		.setImageUsage(usage)
		.setMinImageCount(m_ImageCount)
		.setPresentMode(m_PresentMode)
		.setPreTransform(preTransform)
		.setSurface(m_Device.GetSurface());
	if (queueIndices.GraphicQueueIndex != queueIndices.PresentQueueIndex)
	{
		std::array<uint32_t, 2> indices = { queueIndices.GraphicQueueIndex.value(), queueIndices.PresentQueueIndex.value() };
		swapChainInfo.setQueueFamilyIndexCount(indices.size()).setPQueueFamilyIndices(indices.data());
	}

	VK_CHECK_RESULT(m_Device.GetLogicDevice().createSwapchainKHR(&swapChainInfo, nullptr, &m_SwapChain));

	m_Images = m_Device.GetLogicDevice().getSwapchainImagesKHR(m_SwapChain);
	m_ImageViews.resize(m_Images.size());
	uint32_t imageIndex = 0;
	for (const auto& image : m_Images)
	{
		//m_ImageViews.emplace_back(m_Device.GetLogicDevice(), image, m_Format, vk::ImageAspectFlagBits::eColor, 1, vk::ImageViewType::e2D);
		vk::ImageSubresourceRange region;
		region.setAspectMask(vk::ImageAspectFlagBits::eColor)
			  .setBaseArrayLayer(0)
			  .setBaseMipLevel(0)
			  .setLayerCount(1)
			  .setLevelCount(1);
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
		viewInfo.setComponents(vk::ComponentMapping())
			    .setFormat(m_Format)
			    .setImage(image)
			    .setSubresourceRange(region)
			    .setViewType(vk::ImageViewType::e2D);
		VK_CHECK_RESULT(m_Device.GetLogicDevice().createImageView(&viewInfo, nullptr, &m_ImageViews[imageIndex++]));
	}
}

void SwapChain::ReCreate()
{
	int width, height;
	m_Window.GetFrameBufferSize(&width, &height);
	while (width == 0 || height == 0)
	{
		m_Window.GetFrameBufferSize(&width, &height);
		glfwWaitEvents();
	}
	m_Device.GetLogicDevice().waitIdle();
	Clear();
	Create();
	/*CreateColorSources();
	CreateDepthSources();
	CreateFrameBuffer();*/
}

SwapChain::~SwapChain()
{
	Clear();
}

void SwapChain::Clear()
{
	if (!m_ImageViews.empty())
	{
		for (auto& view : m_ImageViews) 
		{
			m_Device.GetLogicDevice().destroyImageView(view);
			/*view.Clear();*/
		}
		//m_ImageViews.clear();
	}
	if (m_SwapChain)
	{
		m_Device.GetLogicDevice().destroySwapchainKHR(m_SwapChain);
	}
}

void SwapChain::AcquireNextImage(uint32_t* imageIndex, vk::Semaphore waitAcquireImage)
{
	auto acquireImageResult = m_Device.GetLogicDevice().acquireNextImageKHR(m_SwapChain, (std::numeric_limits<uint64_t>::max)(), waitAcquireImage, VK_NULL_HANDLE, imageIndex);
	if (acquireImageResult == vk::Result::eErrorOutOfDateKHR || acquireImageResult == vk::Result::eSuboptimalKHR || m_Window.GetWindowResized())
	{
		m_Window.SetWindowResized(false);
		ReCreate();
		return;
	}
}

void SwapChain::PresentImage(uint32_t imageIndex, vk::Semaphore waitDrawFinish)
{
	vk::PresentInfoKHR presentInfo;
	presentInfo.sType = vk::StructureType::ePresentInfoKHR;
	presentInfo.setPImageIndices(&imageIndex)
		.setPResults(nullptr)
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&waitDrawFinish)
		.setSwapchainCount(1)
		.setPSwapchains(&m_SwapChain);
	auto presentResult = m_Device.GetPresentQueue().presentKHR(&presentInfo);
	if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || m_Window.GetWindowResized())
	{
		m_Window.SetWindowResized(false);
		ReCreate();
	}
}