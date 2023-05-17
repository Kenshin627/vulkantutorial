#include "Application.h"
#include <set>
#include <limits>
#include "../utils/readFile.h"

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
	CreateRenderPass();
	CreateFrameBuffer();
	CreatePipeLine();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateVertexBuffer();
	CreateIndexBuffer();
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
	vk::SubpassDependency dependency;
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			  .setDstSubpass(0)
			  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			  .setSrcAccessMask(vk::AccessFlagBits::eNone)
			  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(1)
				  .setPAttachments(&attachment)
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
					 .setFrontFace(vk::FrontFace::eClockwise)
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
	depthStencilInfo.setDepthTestEnable(VK_FALSE);

	//7.
	vk::PipelineLayoutCreateInfo layoutInfo;
	layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	layoutInfo.setSetLayoutCount(0)
			  .setPSetLayouts(nullptr)
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
		vk::ImageView attachments[] = {view};
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
		framebufferInfo.setAttachmentCount(1)
					   .setPAttachments(&view)
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
		vk::ClearValue clearColor;
		
		vk::Extent2D extent = m_SwapChain.Extent;
		vk::Rect2D renderArea;
		renderArea.setOffset(vk::Offset2D(0, 0))
				  .setExtent(extent);
		vk::RenderPassBeginInfo renderPassBegin;
		renderPassBegin.sType = vk::StructureType::eRenderPassBeginInfo;
		renderPassBegin.setClearValueCount(1)
					   .setPClearValues(&clearColor)
					   .setFramebuffer(m_FrameBuffers[imageIndex])
					   .setRenderPass(m_RenderPass)
					   .setRenderArea(renderArea);
		buffer.beginRenderPass(&renderPassBegin, vk::SubpassContents::eInline);
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
			vk::Viewport viewport;
			viewport.setX(0.0f)
					.setY(0.0f)
					.setWidth(extent.width)
					.setHeight(extent.height)
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