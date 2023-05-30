#include "../Core.h"
#include "SwapChain.h"
#include "../Application.h"

void SwapChain::Init(Device& device, const Window& window, vk::SampleCountFlagBits sampleBits, bool vSync, bool hasDepth)
{
	m_Device = device;
	m_CommandManager = device.GetCommandManager();
	m_Window = window;
	m_vSync = vSync;
	m_Samples = sampleBits;
	m_HasDepth = hasDepth;
	CreateRenderPass();
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
		swapChainInfo.setQueueFamilyIndexCount(static_cast<uint32_t>(indices.size())).setPQueueFamilyIndices(indices.data());
	}

	VK_CHECK_RESULT(m_Device.GetLogicDevice().createSwapchainKHR(&swapChainInfo, nullptr, &m_SwapChain));

	m_SwapChainImages = m_Device.GetLogicDevice().getSwapchainImagesKHR(m_SwapChain);
	m_SwapChainImageViews.resize(m_SwapChainImages.size());
	uint32_t imageIndex = 0;
	for (const auto& image : m_SwapChainImages)
	{
		m_SwapChainImageViews[imageIndex++].Create(m_Device.GetLogicDevice(), image, m_Format);
	}
	/*if (m_Samples != vk::SampleCountFlagBits::e1)
	{*/
	CreateColorAttachment();
	//}
	if (m_HasDepth)
	{
		CreateDepthStencilAttachment();
	}
	CreateFrameBuffers();
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
}

SwapChain::~SwapChain()
{
	Clear();
}

void SwapChain::Clear()
{
	//if (m_Samples != vk::SampleCountFlagBits::e1)
	//{
	for (auto& image : m_ColorAttachments)
	{
		image.Clear();
	}
	//}

	if (m_HasDepth)
	{
		for (auto& image : m_DepthAttachments)
		{
			image.Clear();
		}
	}

	if (!m_FrameBuffers.empty())
	{
		for (auto& fb : m_FrameBuffers) 
		{
			fb.Clear();
		}
	}

	if (!m_SwapChainImageViews.empty())
	{
		for (auto& view : m_SwapChainImageViews)
		{
			view.Clear();
		}
		//m_ImageViews.clear();
	}

	if (m_SwapChain)
	{
		m_Device.GetLogicDevice().destroySwapchainKHR(m_SwapChain);
	}
}

void SwapChain::AcquireNextImage(uint32_t* imageIndex, vk::Semaphore waitAcquireImage, Application* app)
{
	auto acquireImageResult = m_Device.GetLogicDevice().acquireNextImageKHR(m_SwapChain, (std::numeric_limits<uint64_t>::max)(), waitAcquireImage, VK_NULL_HANDLE, imageIndex);
	if (acquireImageResult == vk::Result::eErrorOutOfDateKHR || acquireImageResult == vk::Result::eSuboptimalKHR || m_Window.GetWindowResized())
	{
		m_Window.SetWindowResized(false);
		ReCreate();
		app->CreateSetLayout();
		app->BuildAndUpdateDescriptorSets();
		return;
	}
}

void SwapChain::PresentImage(uint32_t imageIndex, vk::Semaphore waitDrawFinish, Application* app)
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
		app->CreateSetLayout();
		app->BuildAndUpdateDescriptorSets();
	}
}

void SwapChain::CreateColorAttachment()
{
	vk::Extent3D size(m_Extent.width, m_Extent.height, 1);
	m_ColorAttachments.resize(m_ImageCount);
	for (auto& colorAttachment : m_ColorAttachments)
	{
		colorAttachment.Create(m_Device, 1, m_Samples, vk::ImageType::e2D, size, m_Format, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
		colorAttachment.CreateImageView(m_Format);
	}
}

void SwapChain::CreateDepthStencilAttachment()
{
	vk::Format depthFormat = m_Device.FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	vk::Extent3D size(m_Extent.width, m_Extent.height, 1);
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
	if (m_Device.HasStencil(depthFormat))
	{
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}
	m_DepthAttachments.resize(m_ImageCount);
	for (auto& depthAttachment : m_DepthAttachments)
	{
		depthAttachment.Create(m_Device, 1, m_Samples, vk::ImageType::e2D, size, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});

		depthAttachment.CreateImageView(depthFormat, aspectFlags);

		//TODO remove
		depthAttachment.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, aspectFlags);
	}
}

void SwapChain::CreateFrameBuffers()
{
	vk::Device device = m_Device.GetLogicDevice();
	m_FrameBuffers.resize(m_SwapChainImages.size());
	uint32_t index = 0;
	for (auto& view : m_SwapChainImageViews)
	{
		m_FrameBuffers[index].SetAttachment(view.GetVkImageView());
		/*if (m_Samples != vk::SampleCountFlagBits::e1)
		{*/
			m_FrameBuffers[index].SetAttachment(m_ColorAttachments[index].GetVkImageView());
		//}
		
		if (m_HasDepth)
		{
			m_FrameBuffers[index].SetAttachment(m_DepthAttachments[index].GetVkImageView());
		}		
		m_FrameBuffers[index].Create(device, m_Extent.width, m_Extent.height, m_RenderPass);
		index++;
	}
}

void SwapChain::CreateRenderPass()
{
	vk::AttachmentDescription presentAttachment;
	presentAttachment.setFormat(m_Format)
					 .setSamples(vk::SampleCountFlagBits::e1)
					 .setInitialLayout(vk::ImageLayout::eUndefined)
					 .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
					 .setLoadOp(vk::AttachmentLoadOp::eClear)
					 .setStoreOp(vk::AttachmentStoreOp::eStore)
					 .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
					 .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription colorAttachment;
	colorAttachment.setFormat(m_Format)
				   .setSamples(m_Samples)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::Format depthFormat = m_Device.FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	vk::AttachmentDescription depthAttachment;
	depthAttachment.setFormat(depthFormat)
				   .setSamples(m_Samples)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	std::array<vk::AttachmentDescription, 3> attachments = { presentAttachment, colorAttachment, depthAttachment };

	std::array<vk::SubpassDescription, 2> subpassDescs{};

	//first subpass
	vk::AttachmentReference colorReference;
	colorReference.setAttachment(1).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(2).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	subpassDescs[0].setColorAttachmentCount(1)
				   .setPColorAttachments(&colorReference)
				   .setPDepthStencilAttachment(&depthReference)
				   .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	//second subpass
	vk::AttachmentReference colorReferenceSwapchain;
	colorReferenceSwapchain.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference colorInput;
	colorInput.setAttachment(1).setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::AttachmentReference depthInput;
	depthInput.setAttachment(2).setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	std::array<vk::AttachmentReference, 2> inputAttachments = { colorInput, depthInput };
	
	subpassDescs[1].setColorAttachmentCount(1)
				   .setPColorAttachments(&colorReferenceSwapchain)
				   .setInputAttachmentCount(static_cast<uint32_t>(inputAttachments.size()))
				   .setPInputAttachments(inputAttachments.data())
				   .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	std::array<vk::SubpassDependency, 3> dependencies;
	dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
				   .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
				   .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
				   .setDstSubpass(0)
				   .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
				   .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
				   .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

	dependencies[1].setSrcSubpass(0)
				   .setDstSubpass(1)
				   .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				   .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				   .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				   .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
				   .setDependencyFlags(vk::DependencyFlagBits::eByRegion);


	dependencies[2].setSrcSubpass(0)
				   .setDstSubpass(VK_SUBPASS_EXTERNAL)
				   .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				   .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
				   .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
				   .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
				   .setDependencyFlags(vk::DependencyFlagBits::eByRegion);
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
				  .setPAttachments(attachments.data())
				  .setDependencyCount(static_cast<uint32_t>(dependencies.size()))
				  .setPDependencies(dependencies.data())
				  .setSubpassCount(static_cast<uint32_t>(subpassDescs.size()))
				  .setPSubpasses(subpassDescs.data());

	VK_CHECK_RESULT(m_Device.GetLogicDevice().createRenderPass(&renderPassInfo, nullptr, &m_RenderPass));
}