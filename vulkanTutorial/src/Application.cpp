#include "Application.h"
#include <set>
#include <limits>

const static uint32_t WIDTH = 1024, HEIGHT = 728;

void Application::Run()
{
	InitWindow();
	InitVulkan();
	RenderLoop();
	Clear();
}

void Application::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Application::InitVulkan()
{
	CreateInstance();
	CreateSurface();
	PickupPhysicalDevice();
	CreateLogicDevice();
	CreateSwapchain();
}

void Application::RenderLoop()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();
	}
}

void Application::Clear()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::CreateInstance()
{
	vk::ApplicationInfo appInfo;
	appInfo.sType = vk::StructureType::eApplicationInfo;
	appInfo.setApiVersion(VK_API_VERSION_1_3);

	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	

	vk::InstanceCreateInfo instanceInfo;
	instanceInfo.sType = vk::StructureType::eInstanceCreateInfo;
	instanceInfo.setPApplicationInfo(&appInfo)
		.setEnabledExtensionCount(extensionCount)
		.setPpEnabledExtensionNames(extensions)
		.setEnabledLayerCount(0)
		.setPpEnabledLayerNames(nullptr);
	if (vk::createInstance(&instanceInfo, nullptr, &m_VKInstance) != vk::Result::eSuccess)
	{
		throw std::runtime_error("vk instance create failed!");
	}
}

void Application::PickupPhysicalDevice()
{
	auto devices = m_VKInstance.enumeratePhysicalDevices();
	for (const auto& device : devices)
	{
		auto properties = device.getProperties();
		auto features = device.getFeatures();
		auto availabelExtensions = device.enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (auto& extension : availabelExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		
		auto indices = QueryQueueFmily(device);

		SwapchainProperties swapchainProperties = QuerySwapchainSupport(device);
		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && features.geometryShader && indices && requiredExtensions.empty() && !swapchainProperties.formats.empty() && !swapchainProperties.presents.empty())
		{
			m_PhysicalDevice = device;
		}
	}
}

void Application::CreateSurface()
{
	vk::Win32SurfaceCreateInfoKHR surfaceInfo;
	surfaceInfo.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
	surfaceInfo.setHwnd(glfwGetWin32Window(m_Window))
			   .setHinstance(GetModuleHandle(nullptr));
	if (m_VKInstance.createWin32SurfaceKHR(&surfaceInfo, nullptr, &m_Surface) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create surface failed!");
	}
}

Application::QueueFamilyIndices Application::QueryQueueFmily(const vk::PhysicalDevice& device)
{
	QueueFamilyIndices indices;
	auto queueFamilies = device.getQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.GraphicFamily = i;
		}
		vk::Bool32 supportPresent = false;
		device.getSurfaceSupportKHR(i, m_Surface, &supportPresent);
		if (supportPresent)
		{
			indices.PresentFamily = i;
		}
	}
	return indices;
}

void Application::CreateLogicDevice()
{
	vk::PhysicalDeviceFeatures features;

	auto queueIndices = QueryQueueFmily(m_PhysicalDevice);
	std::vector<uint32_t> indices = { queueIndices.GraphicFamily.value(), queueIndices.GraphicFamily.value() };
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;
	float priority = 1.0f;
	for (size_t i = 0; i < indices.size(); i++)
	{
		vk::DeviceQueueCreateInfo queueInfo;
		queueInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
		queueInfo.setPQueuePriorities(&priority)
				 .setQueueCount(1)
				 .setQueueFamilyIndex(indices[i]);
		queueInfos.push_back(queueInfo);
	}

	vk::DeviceCreateInfo deviceInfo;
	deviceInfo.sType = vk::StructureType::eDeviceCreateInfo;
	deviceInfo.setEnabledExtensionCount(m_DeviceExtensions.size())
			  .setPpEnabledExtensionNames(m_DeviceExtensions.data())
			  .setPEnabledFeatures(&features)
			  .setQueueCreateInfoCount(queueInfos.size())
			  .setPQueueCreateInfos(queueInfos.data());
	if (m_PhysicalDevice.createDevice(&deviceInfo, nullptr, &m_Device) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create device failed!");
	}
	m_GraphicQueue = m_Device.getQueue(queueIndices.GraphicFamily.value(), 0);
	m_PresentQueue = m_Device.getQueue(queueIndices.PresentFamily.value(), 0);
}

Application::SwapchainProperties Application::QuerySwapchainSupport(const vk::PhysicalDevice& device)
{
	SwapchainProperties properties;
	properties.capabilities = device.getSurfaceCapabilitiesKHR(m_Surface);
	properties.formats = device.getSurfaceFormatsKHR(m_Surface);
	properties.presents = device.getSurfacePresentModesKHR(m_Surface);
	return properties;
}

void Application::CreateSwapchain()
{
	SwapchainProperties swapchainproperties = QuerySwapchainSupport(m_PhysicalDevice);
	vk::SurfaceCapabilitiesKHR capability = swapchainproperties.capabilities;
	vk::Extent2D extent;
	if (capability.currentExtent.width != (std::numeric_limits<uint32_t>::max)() )
	{
		extent = capability.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		extent.width = static_cast<uint32_t>(width);
		extent.height = static_cast<uint32_t>(height);
		extent.width = std::clamp(extent.width, capability.minImageExtent.width, capability.maxImageExtent.width);
		extent.height = std::clamp(extent.height, capability.minImageExtent.height, capability.maxImageExtent.height);
	}

	uint32_t minImageCount = capability.minImageCount + 1;
	if (capability.maxImageCount > 0 && minImageCount > capability.maxImageCount)
	{
		minImageCount = capability.maxImageCount;
	}

	
	vk::SurfaceFormatKHR format = swapchainproperties.formats[0];
	for (auto& f : swapchainproperties.formats)
	{
		if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			format = f;
		}
	}

	vk::PresentModeKHR present = vk::PresentModeKHR::eFifo;
	for (auto& p : swapchainproperties.presents)
	{
		if (p == vk::PresentModeKHR::eMailbox)
		{
			present = p;
		}
	}

	auto queueIndices = QueryQueueFmily(m_PhysicalDevice);
	uint32_t queueFamilies[] = { queueIndices.GraphicFamily.value(), queueIndices.PresentFamily.value() };

	vk::SwapchainCreateInfoKHR swapChainInfo;
	swapChainInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
	swapChainInfo.setClipped(true)
				 .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
				 .setImageArrayLayers(1)
				 .setImageColorSpace(format.colorSpace)
				 .setImageExtent(extent)
				 .setImageFormat(format.format)
				 .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
				 .setOldSwapchain(VK_NULL_HANDLE)
				 .setPresentMode(present)
				 .setPreTransform(capability.currentTransform)
				 .setSurface(m_Surface).setMinImageCount(minImageCount);
	if (queueIndices.GraphicFamily != queueIndices.PresentFamily)
	{
		swapChainInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
					 .setQueueFamilyIndexCount(2)
					 .setPQueueFamilyIndices(queueFamilies);
	}
	else
	{
		swapChainInfo.setImageSharingMode(vk::SharingMode::eExclusive);
	}

	if (m_Device.createSwapchainKHR(&swapChainInfo, nullptr, &m_SwapChain.vk_SwapChain) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create Swapchain failed!");
	}

	m_SwapChain.Images = m_Device.getSwapchainImagesKHR(m_SwapChain.vk_SwapChain);
	m_SwapChain.ImageViews.resize(m_SwapChain.Images.size());

	vk::ImageSubresourceRange region;
	region.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLayerCount(1)
		.setLevelCount(1);
	uint32_t index = 0;
	for (auto& image : m_SwapChain.Images)
	{
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
		viewInfo.setComponents(vk::ComponentMapping())
				.setFormat(format.format)
				.setImage(image)
				.setViewType(vk::ImageViewType::e2D)
				.setSubresourceRange(region);
		if (m_Device.createImageView(&viewInfo, nullptr, &m_SwapChain.ImageViews[index++]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("create imageView failed!");
		}
	}
	m_SwapChain.vk_Capabilities = capability;
	m_SwapChain.vk_Format = format;
	m_SwapChain.vk__Present = present;
}

void Application::CreateRenderPass()
{
	vk::AttachmentDescription attachment;
	attachment.setFormat(m_SwapChain.vk_Format.format)
			  .setSamples(vk::SampleCountFlagBits::e1)
			  .setInitialLayout(vk::ImageLayout::eUndefined)
			  .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)		
			  .setLoadOp(vk::AttachmentLoadOp::eClear)
			  .setStoreOp(vk::AttachmentStoreOp::eStore)
			  .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			  .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentReference reference;
	reference.setAttachment(0)
			 .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDescription subpassInfo;
	subpassInfo.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			   .setColorAttachmentCount(1)
			   .setPColorAttachments(&reference);
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(1)
			      .setPAttachments(&attachment)
			      .setPSubpasses(&subpassInfo)
			      .setSubpassCount(1);
	if (m_Device.createRenderPass(&renderPassInfo, nullptr, &m_RenderPass) != vk::Result::eSuccess)
	{
		throw std::runtime_error("renderpass create failed!");
	}
}

void Application::CreatePipeLine()
{
	
}