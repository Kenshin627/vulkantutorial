#include "Application.h"
#include <set>
#include <limits>
#include <chrono>

#include <gtc/matrix_transform.hpp>
#include "../utils/readFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Application::InitVulkan()
{
	CreateInstance();
	CreateSurface();
	PickupPhysicalDevice();
	CreateLogicDevice();
	CreateSwapchain();
	CreateCommandPool();
	CreateDepthSources();
	CreateRenderPass();
	CreateFrameBuffer();
	CreateSetLayout();
	CreatePipeLine();
	
	CreateCommandBuffer();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateImageTexture();
	CreateSampler();
	CreateDescriptorPool();
	CreateDescriptorSet();
	UpdateDescriptorSet();
	CreateAsyncObjects();
}

void Application::RenderLoop()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();
		DrawFrame();
	}
	m_Device.waitIdle();
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
			break;
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
		if (indices)
		{
			break;
		}
	}
	return indices;
}

void Application::CreateLogicDevice()
{
	vk::PhysicalDeviceFeatures features;

	auto queueIndices = QueryQueueFmily(m_PhysicalDevice);
	std::vector<uint32_t> indices = { queueIndices.GraphicFamily.value(), queueIndices.PresentFamily.value() };
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
	deviceInfo.setEnabledExtensionCount(static_cast<uint32_t>(m_DeviceExtensions.size()))
			  .setPpEnabledExtensionNames(m_DeviceExtensions.data())
			  .setPEnabledFeatures(&features)
			  .setQueueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
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
			break;
		}
	}

	vk::PresentModeKHR present = vk::PresentModeKHR::eFifo;
	for (auto& p : swapchainproperties.presents)
	{
		if (p == vk::PresentModeKHR::eMailbox)
		{
			present = p;
			break;
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
				 .setSurface(m_Surface)
				 .setMinImageCount(minImageCount);
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
	m_SwapChain.vk_Present = present;
	m_SwapChain.Extent = extent;
}

void Application::CreateRenderPass()
{
	vk::AttachmentDescription colorAttachment;
	colorAttachment.setFormat(m_SwapChain.vk_Format.format)
				   .setSamples(vk::SampleCountFlagBits::e1)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)		
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription depthAttachment;
	depthAttachment.setFormat(vk::Format::eD32Sfloat)
				   .setSamples(vk::SampleCountFlagBits::e1)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eDontCare)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	vk::AttachmentReference colorReference;
	colorReference.setAttachment(0)
				  .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(1)
			      .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	
	vk::SubpassDescription subpassInfo;
	subpassInfo.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			   .setColorAttachmentCount(1)
			   .setPColorAttachments(&colorReference)
			   .setPDepthStencilAttachment(&depthReference);
	vk::SubpassDependency dependency;
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			  .setDstSubpass(0)
			  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			  .setSrcAccessMask(vk::AccessFlagBits::eNone)
			  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(attachments.size())
				  .setPAttachments(attachments.data())
				  .setPSubpasses(&subpassInfo)
				  .setSubpassCount(1)
				  .setDependencyCount(1)
				  .setPDependencies(&dependency);
	if (m_Device.createRenderPass(&renderPassInfo, nullptr, &m_RenderPass) != vk::Result::eSuccess)
	{
		throw std::runtime_error("renderpass create failed!");
	}
}

void Application::CreatePipeLine()
{
	//1.
	auto bindings = Vertex::GetBindingDescription();
	auto attributes = Vertex::GetAttributes();
	vk::PipelineVertexInputStateCreateInfo vertexInput;
	vertexInput.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
	vertexInput.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributes.size()))
			   .setPVertexAttributeDescriptions(attributes.data())
			   .setVertexBindingDescriptionCount(1)
			   .setPVertexBindingDescriptions(&bindings);

	//2.
	vk::PipelineInputAssemblyStateCreateInfo assemblyInfo;
	assemblyInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
	assemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList)
			    .setPrimitiveRestartEnable(VK_FALSE);

	//3.
	vk::ShaderModule vertexShader = CompilerShader("resource/shaders/vert.spv");
	vk::PipelineShaderStageCreateInfo vertexInfo;
	vertexInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	vertexInfo.setModule(vertexShader)
			  .setPName("main")
			  .setStage(vk::ShaderStageFlagBits::eVertex);
	vk::ShaderModule fragmentShader = CompilerShader("resource/shaders/frag.spv");
	vk::PipelineShaderStageCreateInfo fragmentInfo;
	fragmentInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	fragmentInfo.setModule(fragmentShader)
				.setPName("main")
				.setStage(vk::ShaderStageFlagBits::eFragment);
	vk::PipelineShaderStageCreateInfo shaders[] = { vertexInfo, fragmentInfo };

	//4.
	vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
	rasterizationInfo.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
	rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack)
					 .setDepthBiasClamp(0.0f)
					 .setDepthBiasConstantFactor(0.0f)
					 .setDepthBiasEnable(VK_FALSE)
					 .setDepthBiasSlopeFactor(0.0f)
					 .setDepthClampEnable(VK_FALSE)
					 .setFrontFace(vk::FrontFace::eCounterClockwise)
					 .setLineWidth(1.0f)
					 .setPolygonMode(vk::PolygonMode::eFill)
					 .setRasterizerDiscardEnable(VK_FALSE);

	//5.
	vk::PipelineColorBlendAttachmentState attachment;
	attachment.setBlendEnable(VK_FALSE)
			  .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
	vk::PipelineColorBlendStateCreateInfo blendingInfo;
	blendingInfo.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
	blendingInfo.setAttachmentCount(1)
			    .setPAttachments(&attachment)
			    .setLogicOpEnable(VK_FALSE);

	//6.
	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
	depthStencilInfo.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
	depthStencilInfo.setDepthTestEnable(VK_TRUE)
		.setBack({})
		.setDepthBoundsTestEnable(VK_FALSE)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthWriteEnable(VK_TRUE)
		.setFront({})
		.setMaxDepthBounds(1.0f)
		.setMinDepthBounds(0.0f)
		.setStencilTestEnable(VK_FALSE);

	//7.
	vk::PipelineLayoutCreateInfo layoutInfo;
	layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	layoutInfo.setSetLayoutCount(1)
			  .setPSetLayouts(&m_SetLayout)
			  .setPushConstantRangeCount(0)
			  .setPPushConstantRanges(nullptr);  
	if (m_Device.createPipelineLayout(&layoutInfo, nullptr, &m_Layout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create pipeline layout faield!");
	}

	//8.
	vk::PipelineViewportStateCreateInfo viewportInfo;
	viewportInfo.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
	viewportInfo.setViewportCount(1)
		        .setScissorCount(1);

	//9.
	std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
	dynamicState.setDynamicStateCount(dynamicStates.size())
		.setPDynamicStates(dynamicStates.data());

	//10.
	vk::PipelineMultisampleStateCreateInfo multisamplesInfo;
	multisamplesInfo.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
	multisamplesInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			        .setMinSampleShading(1.0f)
			        .setSampleShadingEnable(VK_FALSE)
			        .setAlphaToCoverageEnable(VK_FALSE)
			        .setAlphaToOneEnable(VK_FALSE)
			        .setPSampleMask(nullptr);

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
	pipelineInfo.setPVertexInputState(&vertexInput)
				.setPInputAssemblyState(&assemblyInfo)
				.setStageCount(2)
				.setPStages(shaders)
				.setPRasterizationState(&rasterizationInfo)
				.setPViewportState(&viewportInfo)
				.setPColorBlendState(&blendingInfo)
				.setPDepthStencilState(&depthStencilInfo)
				.setPMultisampleState(&multisamplesInfo)
				.setLayout(m_Layout)
				.setRenderPass(m_RenderPass)
				.setSubpass(0)
				.setPDynamicState(&dynamicState)
				.setBasePipelineHandle(VK_NULL_HANDLE)
				.setBasePipelineIndex(-1);
	if (m_Device.createGraphicsPipelines({}, 1, &pipelineInfo, nullptr, &m_Pipeline) != vk::Result::eSuccess)
	{
		throw std::runtime_error("pipeline Create failed!");
	}
}

vk::ShaderModule Application::CompilerShader(const std::string& path)
{
	vk::ShaderModule result;
	auto shaderCode = ReadFile(path);
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
	shaderInfo.setCodeSize(shaderCode.size())
			  .setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()));
	if (m_Device.createShaderModule(&shaderInfo, nullptr, &result) != vk::Result::eSuccess)
	{
		throw std::runtime_error("shader module create failed!");
	}
	return result;
}

void Application::CreateFrameBuffer()
{
	m_FrameBuffers.resize(m_SwapChain.ImageViews.size());
	uint32_t index = 0;
	for (auto& view : m_SwapChain.ImageViews)
	{
		std::array<vk::ImageView, 2> attachments = { view, m_DepthImageView };
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
		framebufferInfo.setAttachmentCount(attachments.size())
					   .setPAttachments(attachments.data())
					   .setWidth(m_SwapChain.Extent.width)
					   .setHeight(m_SwapChain.Extent.height)
					   .setLayers(1)
					   .setRenderPass(m_RenderPass);
		if (m_Device.createFramebuffer(&framebufferInfo, nullptr, &m_FrameBuffers[index++]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("frameBuffer create failed!");
		}
	}
}

void Application::CreateCommandPool()
{
	auto queueFamilyIndices = QueryQueueFmily(m_PhysicalDevice);
	vk::CommandPoolCreateInfo commandPoolInfo;
	commandPoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
	commandPoolInfo.setQueueFamilyIndex(queueFamilyIndices.GraphicFamily.value())
				   .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	if (m_Device.createCommandPool(&commandPoolInfo, nullptr, &m_CommandPool) != vk::Result::eSuccess)
	{
		throw std::runtime_error("commandPool create failed!");
	}
}

void Application::CreateCommandBuffer()
{
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	commandBufferInfo.setCommandBufferCount(1)
					 .setCommandPool(m_CommandPool)
					 .setLevel(vk::CommandBufferLevel::ePrimary);
	if (m_Device.allocateCommandBuffers(&commandBufferInfo, &m_CommandBuffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("commandBuffer create failed!");
	}
}

void Application::RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex)
{
	vk::CommandBufferBeginInfo commandBufferBegin;
	commandBufferBegin.sType = vk::StructureType::eCommandBufferBeginInfo;
	commandBufferBegin.setPInheritanceInfo(nullptr);
					  //.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	buffer.begin(&commandBufferBegin);
		std::array<vk::ClearValue, 2> clearValues{};
		clearValues[0].color = vk::ClearColorValue();
		clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0.0f);
		
		
		vk::Extent2D extent = m_SwapChain.Extent;
		vk::Rect2D renderArea;
		renderArea.setOffset(vk::Offset2D(0, 0))
				  .setExtent(extent);
		vk::RenderPassBeginInfo renderPassBegin;
		renderPassBegin.sType = vk::StructureType::eRenderPassBeginInfo;
		renderPassBegin.setClearValueCount(clearValues.size())
					   .setPClearValues(clearValues.data())
					   .setFramebuffer(m_FrameBuffers[imageIndex])
					   .setRenderPass(m_RenderPass)
					   .setRenderArea(renderArea);
		buffer.beginRenderPass(&renderPassBegin, vk::SubpassContents::eInline);
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
			vk::Viewport viewport;
			viewport.setX(0.0f)
					.setY(0.0f)
					.setWidth((float)extent.width)
					.setHeight((float)extent.height)
					.setMinDepth(0.0f)
					.setMaxDepth(1.0f);
			vk::Rect2D scissor;
			scissor.setOffset({ 0, 0 })
				   .setExtent(extent);
			buffer.setViewport(0, 1, &viewport);
			buffer.setScissor(0, 1, &scissor);
			vk::DeviceSize size(0);
			buffer.bindVertexBuffers(0, 1, &m_VertexBuffer, &size);
			buffer.bindIndexBuffer(m_IndexBuffer, 0, vk::IndexType::eUint16);
			buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Layout, 0, 1, &m_DescriptorSet, 0, nullptr);
			buffer.drawIndexed(m_Indices.size(), 1, 0, 0, 0);
		buffer.endRenderPass();
	buffer.end();
}				

void Application::DrawFrame()
{
	m_Device.waitForFences(1, &m_InFlightFence, VK_TRUE, (std::numeric_limits<uint64_t>::max)());
	m_Device.resetFences(1, &m_InFlightFence);

	uint32_t imageIndex;
	m_Device.acquireNextImageKHR(m_SwapChain.vk_SwapChain, (std::numeric_limits<uint64_t>::max)(), m_WaitAcquireImageSemaphore, VK_NULL_HANDLE, &imageIndex);
	m_CommandBuffer.reset();
	RecordCommandBuffer(m_CommandBuffer, imageIndex);

	UpdateUniformBuffers();

	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::SubmitInfo submitInfo;
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&m_CommandBuffer)
			  .setWaitSemaphoreCount(1)
			  .setWaitSemaphoreCount(1)
			  .setPWaitSemaphores(&m_WaitAcquireImageSemaphore)
			  .setSignalSemaphoreCount(1)
			  .setPSignalSemaphores(&m_WaitFinishDrawSemaphore)
			  .setPWaitDstStageMask(waitStages);
	m_GraphicQueue.submit(1, &submitInfo, m_InFlightFence);

	vk::PresentInfoKHR presentInfo;
	presentInfo.sType = vk::StructureType::ePresentInfoKHR;
	presentInfo.setPImageIndices(&imageIndex)
			   .setPResults(nullptr)
			   .setWaitSemaphoreCount(1)
			   .setPWaitSemaphores(&m_WaitFinishDrawSemaphore)
			   .setSwapchainCount(1)
			   .setPSwapchains(&m_SwapChain.vk_SwapChain);
	m_PresentQueue.presentKHR(&presentInfo);
}

void Application::CreateAsyncObjects()
{
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

	vk::SemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;
	if (m_Device.createFence(&fenceInfo, nullptr, &m_InFlightFence) != vk::Result::eSuccess || m_Device.createSemaphore(&semaphoreInfo, nullptr, &m_WaitAcquireImageSemaphore) != vk::Result::eSuccess || m_Device.createSemaphore(&semaphoreInfo, nullptr, &m_WaitFinishDrawSemaphore) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create asyncObjects failed!");
	}
}

void Application::CreateVertexBuffer()
{
	vk::DeviceSize size = sizeof(m_VertexData[0]) * m_VertexData.size();
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;
	
	CreateBuffer(stagingBuffer, stagingMemory, size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	void* data;
	m_Device.mapMemory(stagingMemory, { 0 }, size, {}, &data);
	memcpy(data, m_VertexData.data(), size);
	m_Device.unmapMemory(stagingMemory);

	CreateBuffer(m_VertexBuffer, m_VertexMemory, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal);

	auto command = OneTimeSubmitCommandBegin();

	vk::BufferCopy region;
	region.setDstOffset({ 0 })
		  .setSrcOffset({ 0 })
		  .setSize(size);
	command.copyBuffer(stagingBuffer, m_VertexBuffer, 1, &region);

	OneTimeSubmitCommandEnd(command);	
	m_Device.destroyBuffer(stagingBuffer);
	m_Device.freeMemory(stagingMemory);
}

void Application::CreateIndexBuffer()
{
	vk::DeviceSize size = sizeof(uint16_t) * m_Indices.size();
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;
	CreateBuffer(stagingBuffer, stagingMemory, size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible |		vk::MemoryPropertyFlagBits::eHostCoherent);
	void* data;
	m_Device.mapMemory(stagingMemory, 0, size, {}, &data);
	memcpy(data, m_Indices.data(), size);
	m_Device.unmapMemory(stagingMemory);

	CreateBuffer(m_IndexBuffer, m_IndexMemory, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal);

	auto command = OneTimeSubmitCommandBegin();
	vk::BufferCopy region;
	region.setDstOffset(0)
		  .setSrcOffset(0)
		  .setSize(size);
	command.copyBuffer(stagingBuffer, m_IndexBuffer, 1, &region);
	OneTimeSubmitCommandEnd(command);
}

uint32_t Application::FindMemoryPropertyType(uint32_t memoryType, vk::MemoryPropertyFlags flags)
{
	auto memoryProperties = m_PhysicalDevice.getMemoryProperties();
	for (size_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((memoryType &  (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags))
		{
			return i;
		}
	}
}

void Application::CreateBuffer(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::SharingMode sharingMode, vk::MemoryPropertyFlags memoryPropertyFlags)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	bufferInfo.setSharingMode(sharingMode)
			  .setSize(size)
			  .setUsage(usage);
	if (m_Device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("buffer create failed!");
	}

	vk::MemoryRequirements requirement;
	m_Device.getBufferMemoryRequirements(buffer, &requirement);
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(requirement.size)
			  .setMemoryTypeIndex(FindMemoryPropertyType(requirement.memoryTypeBits, memoryPropertyFlags));
	if (m_Device.allocateMemory(&memoryInfo, nullptr, &memory) != vk::Result::eSuccess)
	{
		throw std::runtime_error("allocate memory failed!");
	}
	m_Device.bindBufferMemory(buffer, memory, 0);
}

vk::CommandBuffer Application::OneTimeSubmitCommandBegin()
{
	vk::CommandBuffer commandBuffer;
	vk::CommandBufferAllocateInfo allocateInfo;
	allocateInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	allocateInfo.setCommandBufferCount(1)
				.setCommandPool(m_CommandPool)
				.setLevel(vk::CommandBufferLevel::ePrimary);
	if (m_Device.allocateCommandBuffers(&allocateInfo, &commandBuffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("commandBuffer allocate failed!");
	}
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			 .setPInheritanceInfo(nullptr);
	if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
	{
		throw std::runtime_error("commandBuffer begin failed!");
	}
	return commandBuffer;
}

void Application::OneTimeSubmitCommandEnd(vk::CommandBuffer command)
{
	command.end();
	vk::SubmitInfo submitInfo;
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&command);
	if (m_GraphicQueue.submit(1, &submitInfo, VK_NULL_HANDLE) != vk::Result::eSuccess)
	{
		throw std::runtime_error("submit command failed!");
	}
	m_GraphicQueue.waitIdle();
}

void Application::CreateSetLayout()
{
	vk::DescriptorSetLayoutBinding uniformBinding;
	uniformBinding.setBinding(0)
		          .setDescriptorCount(1)
		          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
		          .setPImmutableSamplers(nullptr)
		          .setStageFlags(vk::ShaderStageFlagBits::eVertex);

	vk::DescriptorSetLayoutBinding samplerBinding;
	samplerBinding.setBinding(1)
				  .setDescriptorCount(1)
				  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				  .setPImmutableSamplers(nullptr)
				  .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uniformBinding, samplerBinding };

	vk::DescriptorSetLayoutCreateInfo setlayoutInfo;
	setlayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
	setlayoutInfo.setBindingCount(bindings.size())
				 .setPBindings(bindings.data());
	if (m_Device.createDescriptorSetLayout(&setlayoutInfo, nullptr, &m_SetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("descriptor setlayout create failed!");
	}
}

void Application::CreateUniformBuffer()
{
	vk::DeviceSize size = sizeof(UniformBufferObject);
	CreateBuffer(m_UniformBuffer, m_UniformMemory, size, vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	if (m_Device.mapMemory(m_UniformMemory, 0, size, {}, &m_UniformMappedData) != vk::Result::eSuccess)
	{
		throw std::runtime_error("map uniformBuffer memory failed!");
	}
}

void Application::UpdateUniformBuffers()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.Proj = glm::perspective(glm::radians(45.0f), m_SwapChain.Extent.width / (float)m_SwapChain.Extent.height, 0.1f, 10.0f);
	ubo.Proj[1][1] *= -1;
	ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	memcpy(m_UniformMappedData, &ubo, sizeof(UniformBufferObject));
}

void Application::CreateDescriptorPool()
{
	vk::DescriptorPoolSize uniformPoolSize;
	uniformPoolSize.setDescriptorCount(1)
				   .setType(vk::DescriptorType::eUniformBuffer);

	vk::DescriptorPoolSize samplerPoolSize;
	samplerPoolSize.setDescriptorCount(1)
				   .setType(vk::DescriptorType::eCombinedImageSampler);

	std::array<vk::DescriptorPoolSize, 2> poolSize = { uniformPoolSize, samplerPoolSize };

	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
	descriptorPoolInfo.setMaxSets(1)
					  .setPoolSizeCount(poolSize.size())
					  .setPPoolSizes(poolSize.data());
	if (m_Device.createDescriptorPool(&descriptorPoolInfo, nullptr, &m_DescriptorPool) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create descriptorPool failed!");
	}
}

void Application::CreateDescriptorSet()
{
	vk::DescriptorSetAllocateInfo setInfo;
	setInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
	setInfo.setDescriptorPool(m_DescriptorPool)
		   .setDescriptorSetCount(1)
		   .setPSetLayouts(&m_SetLayout);
	if (m_Device.allocateDescriptorSets(&setInfo, &m_DescriptorSet) != vk::Result::eSuccess)
	{
		throw std::runtime_error("descriptorSet allocate failed!");
	}
}

void Application::UpdateDescriptorSet()
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.setBuffer(m_UniformBuffer)
			  .setOffset(0)
			  .setRange(sizeof(UniformBufferObject));

	vk::WriteDescriptorSet uniformWriteSet;
	uniformWriteSet.sType = vk::StructureType::eWriteDescriptorSet;
	uniformWriteSet.setDescriptorCount(1)
				   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
				   .setDstArrayElement(0)
				   .setDstBinding(0)
				   .setDstSet(m_DescriptorSet)
				   .setPBufferInfo(&bufferInfo);

	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			 .setImageView(m_ImageView)
			 .setSampler(m_Sampler);

	vk::WriteDescriptorSet samplerWrite;
	samplerWrite.sType = vk::StructureType::eWriteDescriptorSet;
	samplerWrite.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDstArrayElement(0)
		.setDstBinding(1)
		.setDstSet(m_DescriptorSet)
		.setPImageInfo(&imageInfo);
	std::array<vk::WriteDescriptorSet, 2> writes = { uniformWriteSet, samplerWrite };
	m_Device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
}

void Application::CreateImageTexture()
{
	int width, height, channel;
	stbi_uc* pixels = stbi_load("resource/textures/texture.jpg", &width, &height, &channel, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("read imagefile failed!");
	}
	vk::DeviceSize size = width * height * 4;

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;
	CreateBuffer(stagingBuffer, stagingMemory, size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	void* data;
	if (m_Device.mapMemory(stagingMemory, 0, size, {}, &data) != vk::Result::eSuccess)
	{
		throw std::runtime_error("map memory failed!");
	}
	else
	{
		memcpy(data, pixels, size);
		m_Device.unmapMemory(stagingMemory);
	}
	CreateImage(m_Image, m_ImageMemory, m_ImageView, vk::Extent2D(width, height), vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::ImageAspectFlagBits::eColor, vk::MemoryPropertyFlagBits::eDeviceLocal);
	TransiationImageLayout(m_Image, vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);
	CopyBufferToImage(stagingBuffer, m_Image, vk::Extent3D(width, height, 1));
	TransiationImageLayout(m_Image, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);
}

void Application::CreateImage(vk::Image& image, vk::DeviceMemory& memory, vk::ImageView& imageView, vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vk::ImageAspectFlags aspectFlags, vk::MemoryPropertyFlags memoryPropertyFlags)
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.sType = vk::StructureType::eImageCreateInfo;
	imageInfo.setArrayLayers(1)
			 .setExtent(vk::Extent3D(extent.width, extent.height, 1))
			 .setFormat(format)
			 .setImageType(vk::ImageType::e2D)
			 .setInitialLayout(vk::ImageLayout::eUndefined)
			 .setMipLevels(0)
			 .setSamples(vk::SampleCountFlagBits::e1)
			 .setSharingMode(vk::SharingMode::eExclusive)
			 .setTiling(tiling)
			 .setUsage(usage);
	if (m_Device.createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
	{
		throw std::runtime_error("image create failed!");
	}

	vk::MemoryRequirements requirement;
	m_Device.getImageMemoryRequirements(image, &requirement);
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(requirement.size)
			  .setMemoryTypeIndex(FindMemoryPropertyType(requirement.memoryTypeBits, memoryPropertyFlags));
	if (m_Device.allocateMemory(&memoryInfo, nullptr, &memory) != vk::Result::eSuccess)
	{
		throw std::runtime_error("imageMemory allocate failed!");
	}
	m_Device.bindImageMemory(image, memory, 0);

	vk::ImageSubresourceRange region;
	region.setAspectMask(aspectFlags)
		  .setBaseArrayLayer(0)
		  .setBaseMipLevel(0)
		  .setLayerCount(1)
		  .setLevelCount(1);
	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	imageViewInfo.setComponents(vk::ComponentMapping())
				 .setFormat(format)
				 .setImage(image)
				 .setSubresourceRange(region)
				 .setViewType(vk::ImageViewType::e2D);
	if (m_Device.createImageView(&imageViewInfo, nullptr, &imageView) != vk::Result::eSuccess)
	{
		throw std::runtime_error("imageView create failed!");
	}
}

void Application::CopyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, vk::Extent3D extent)
{
	auto command = OneTimeSubmitCommandBegin();

	vk::ImageSubresourceLayers layer;
	layer.setAspectMask(vk::ImageAspectFlagBits::eColor)
		 .setBaseArrayLayer(0)
		 .setLayerCount(1)
		 .setMipLevel(0);
	vk::BufferImageCopy region;
	region.setBufferImageHeight(0)
		  .setBufferOffset(0)
		  .setBufferRowLength(0)
		  .setImageExtent(extent)
		  .setImageOffset(0)
		  .setImageSubresource(layer);
	command.copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eShaderReadOnlyOptimal, 1, &region);

	OneTimeSubmitCommandEnd(command);
}

void Application::TransiationImageLayout(vk::Image image, vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess, vk::ImageLayout srcLayout, vk::PipelineStageFlags dstStage,  vk::AccessFlags dstAccess, vk::ImageLayout dstLayout, vk::ImageAspectFlags aspectFlags)
{
	auto command = OneTimeSubmitCommandBegin();

	vk::ImageSubresourceRange range;
	range.setAspectMask(aspectFlags)
		 .setBaseArrayLayer(0)
		 .setBaseMipLevel(0)
		 .setLayerCount(1)
		 .setLevelCount(1);
	vk::ImageMemoryBarrier barrier;
	barrier.sType = vk::StructureType::eImageMemoryBarrier;
	barrier.setImage(image)
		   .setSrcAccessMask(srcAccess)
		   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		   .setOldLayout(srcLayout)
		   .setDstAccessMask(dstAccess)
		   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		   .setNewLayout(dstLayout)
		   .setSubresourceRange(range);
	command.pipelineBarrier(srcStage, dstStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

	OneTimeSubmitCommandEnd(command);
}

void Application::CreateSampler()
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
			   .setUnnormalizedCoordinates(VK_FALSE);
	if (m_Device.createSampler(&samplerInfo, nullptr, &m_Sampler) != vk::Result::eSuccess)
	{
		throw std::runtime_error("sampler create failed!");
	}
}

void Application::CreateDepthSources()
{
	CreateImage(m_DepthImage, m_DepthMemory, m_DepthImageView, m_SwapChain.Extent, vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vk::ImageAspectFlagBits::eDepth, vk::MemoryPropertyFlagBits::eDeviceLocal);
	TransiationImageLayout(m_DepthImage, vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageAspectFlagBits::eDepth);
}