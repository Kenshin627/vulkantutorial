#include "Application.h"
#include <set>
#include <limits>
#include <chrono>
#include <unordered_map>

#include <gtc/matrix_transform.hpp>
#include "../utils/readFile.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../vendor/tiny_obj_loader/tiny_obj_loader.h"

void Application::Run()
{
	InitVulkan();
	RenderLoop();
	Clear();
}

void Application::InitWindow(int width, int height, const char* title)
{
	m_Window = Window(width, height, title);
}

void Application::InitDevice(Window& window)
{
	m_Device = Device(window, m_VKInstance);
}

void Application::InitVulkan()
{
	CreateInstance();
	InitDevice(m_Window);
	m_SwapChain.Init(m_Device, m_Window, false);

	CreateColorSources();
	CreateDepthSources();
	CreateRenderPass();
	CreateFrameBuffer();
	CreateSetLayout();
	CreatePipeLine();

	CreateImageTexture("resource/textures/vikingRoom.png");
	LoadModel("resource/models/vikingRoom.obj");
	
	CreateCommandBuffer();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateSampler();
	CreateDescriptorPool();
	CreateDescriptorSet();
	UpdateDescriptorSet();
	CreateAsyncObjects();
}

void Application::RenderLoop()
{
	while (!m_Window.ShouldClose())
	{
		m_Window.PollEvents();
		DrawFrame();
	}
	m_Device.GetLogicDevice().waitIdle();
}

void Application::Clear()
{
	
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

void Application::CreateRenderPass()
{
	vk::AttachmentDescription colorAttachment;
	colorAttachment.setFormat(m_SwapChain.GetFormat())
				   .setSamples(m_SamplerCount)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)		
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::Format depthFormat = FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	vk::AttachmentDescription depthAttachment;
	depthAttachment.setFormat(depthFormat)
				   .setSamples(m_SamplerCount)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eDontCare)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription resolvedAttachment;
	resolvedAttachment.setFormat(m_SwapChain.GetFormat())
					  .setSamples(vk::SampleCountFlagBits::e1)
					  .setInitialLayout(vk::ImageLayout::eUndefined)
					  .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
					  .setLoadOp(vk::AttachmentLoadOp::eDontCare)
					  .setStoreOp(vk::AttachmentStoreOp::eStore)
					  .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
					  .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, resolvedAttachment };
	vk::AttachmentReference colorReference;
	colorReference.setAttachment(0)
				  .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(1)
			      .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference resolveReference;
	resolveReference.setAttachment(2)
				    .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	
	vk::SubpassDescription subpassInfo;
	subpassInfo.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			   .setColorAttachmentCount(1)
			   .setPColorAttachments(&colorReference)
			   .setPDepthStencilAttachment(&depthReference)
			   .setPResolveAttachments(&resolveReference);
	vk::SubpassDependency dependency;
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			  .setDstSubpass(0)
			  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			  .setSrcAccessMask(vk::AccessFlagBits::eNone)
			  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
				  .setPAttachments(attachments.data())
				  .setPSubpasses(&subpassInfo)
				  .setSubpassCount(1)
				  .setDependencyCount(1)
				  .setPDependencies(&dependency);
	if (m_Device.GetLogicDevice().createRenderPass(&renderPassInfo, nullptr, &m_RenderPass) != vk::Result::eSuccess)
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
	if (m_Device.GetLogicDevice().createPipelineLayout(&layoutInfo, nullptr, &m_Layout) != vk::Result::eSuccess)
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
	dynamicState.setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
		.setPDynamicStates(dynamicStates.data());

	//10.
	vk::PipelineMultisampleStateCreateInfo multisamplesInfo;
	multisamplesInfo.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
	multisamplesInfo.setRasterizationSamples(m_SamplerCount)
			        .setMinSampleShading(0.2f)
			        .setSampleShadingEnable(VK_TRUE)
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
	if (m_Device.GetLogicDevice().createGraphicsPipelines({}, 1, &pipelineInfo, nullptr, &m_Pipeline) != vk::Result::eSuccess)
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
	if (m_Device.GetLogicDevice().createShaderModule(&shaderInfo, nullptr, &result) != vk::Result::eSuccess)
	{
		throw std::runtime_error("shader module create failed!");
	}
	return result;
}

void Application::CreateFrameBuffer()
{
	m_FrameBuffers.resize(m_SwapChain.GetImageViews().size());
	uint32_t index = 0;
	for (auto& view : m_SwapChain.GetImageViews())
	{
		std::array<vk::ImageView, 3> attachments = { m_ColorView, m_DepthImageView, view };
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
		framebufferInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
					   .setPAttachments(attachments.data())
					   .setWidth(m_SwapChain.GetExtent().width)
					   .setHeight(m_SwapChain.GetExtent().height)
					   .setLayers(1)
					   .setRenderPass(m_RenderPass);
		if (m_Device.GetLogicDevice().createFramebuffer(&framebufferInfo, nullptr, &m_FrameBuffers[index++]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("frameBuffer create failed!");
		}
	}
}

void Application::CreateCommandBuffer()
{
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	commandBufferInfo.setCommandBufferCount(1)
					 .setCommandPool(m_Device.GetCommandPool())
					 .setLevel(vk::CommandBufferLevel::ePrimary);
	if (m_Device.GetLogicDevice().allocateCommandBuffers(&commandBufferInfo, &m_CommandBuffer) != vk::Result::eSuccess)
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

	auto beginRes = buffer.begin(&commandBufferBegin);
		std::array<vk::ClearValue, 2> clearValues{};
		clearValues[0].color = vk::ClearColorValue();
		clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		
		
		vk::Extent2D extent = m_SwapChain.GetExtent();
		vk::Rect2D renderArea;
		renderArea.setOffset(vk::Offset2D(0, 0))
				  .setExtent(extent);
		vk::RenderPassBeginInfo renderPassBegin;
		renderPassBegin.sType = vk::StructureType::eRenderPassBeginInfo;
		renderPassBegin.setClearValueCount(static_cast<uint32_t>(clearValues.size()))
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
			buffer.bindVertexBuffers(0, 1, &m_VertexBuffer.m_Buffer, &size);
			buffer.bindIndexBuffer(m_IndexBuffer.m_Buffer, 0, vk::IndexType::eUint32);
			buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Layout, 0, 1, &m_DescriptorSet, 0, nullptr);
			buffer.drawIndexed(static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
		buffer.endRenderPass();
	buffer.end();
}				

void Application::DrawFrame()
{
	auto fenceResult = m_Device.GetLogicDevice().waitForFences(1, &m_InFlightFence, VK_TRUE, (std::numeric_limits<uint64_t>::max)());

	uint32_t imageIndex;
	m_SwapChain.AcquireNextImage(&imageIndex, m_WaitAcquireImageSemaphore);
	
	auto resetFenceRes = m_Device.GetLogicDevice().resetFences(1, &m_InFlightFence);
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
	auto submitRes = m_Device.GetGraphicQueue().submit(1, &submitInfo, m_InFlightFence);

	m_SwapChain.PresentImage(imageIndex, m_WaitFinishDrawSemaphore);
}

void Application::CreateAsyncObjects()
{
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

	vk::SemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;
	if (m_Device.GetLogicDevice().createFence(&fenceInfo, nullptr, &m_InFlightFence) != vk::Result::eSuccess || m_Device.GetLogicDevice().createSemaphore(&semaphoreInfo, nullptr, &m_WaitAcquireImageSemaphore) != vk::Result::eSuccess || m_Device.GetLogicDevice().createSemaphore(&semaphoreInfo, nullptr, &m_WaitFinishDrawSemaphore) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create asyncObjects failed!");
	}
}

void Application::CreateVertexBuffer()
{
	vk::DeviceSize size = sizeof(m_VertexData[0]) * m_VertexData.size();
	Buffer stagingBuffer;
	m_Device.CreateBuffer(vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &stagingBuffer, m_VertexData.data());

	m_Device.CreateBuffer(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, &m_VertexBuffer, nullptr);

	m_Device.CopyBuffer(stagingBuffer.m_Buffer, 0, m_VertexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue());
}

void Application::CreateIndexBuffer()
{
	vk::DeviceSize size = sizeof(uint32_t) * m_Indices.size();
	Buffer stagingBuffer;
	m_Device.CreateBuffer(vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &stagingBuffer, m_Indices.data());
	m_Device.CreateBuffer(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, &m_IndexBuffer, nullptr);
	m_Device.CopyBuffer(stagingBuffer.m_Buffer, 0, m_IndexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue());
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
	setlayoutInfo.setBindingCount(static_cast<uint32_t>(bindings.size()))
				 .setPBindings(bindings.data());
	if (m_Device.GetLogicDevice().createDescriptorSetLayout(&setlayoutInfo, nullptr, &m_SetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("descriptor setlayout create failed!");
	}
}

void Application::CreateUniformBuffer()
{
	vk::DeviceSize size = sizeof(UniformBufferObject);
	m_Device.CreateBuffer(vk::BufferUsageFlagBits::eUniformBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &m_UniformBuffer, nullptr);
	m_UniformBuffer.Map();
}

void Application::UpdateUniformBuffers()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.Proj = glm::perspective(glm::radians(45.0f), m_SwapChain.GetExtent().width / (float)m_SwapChain.GetExtent().height, 0.1f, 100.0f);
	ubo.Proj[1][1] *= -1;
	ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	/*memcpy(m_UniformMappedData, &ubo, sizeof(UniformBufferObject));*/
	m_UniformBuffer.CopyFrom(&ubo, sizeof(UniformBufferObject));
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
					  .setPoolSizeCount(static_cast<uint32_t>(poolSize.size()))
					  .setPPoolSizes(poolSize.data());
	if (m_Device.GetLogicDevice().createDescriptorPool(&descriptorPoolInfo, nullptr, &m_DescriptorPool) != vk::Result::eSuccess)
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
	if (m_Device.GetLogicDevice().allocateDescriptorSets(&setInfo, &m_DescriptorSet) != vk::Result::eSuccess)
	{
		throw std::runtime_error("descriptorSet allocate failed!");
	}
}

void Application::UpdateDescriptorSet()
{
	vk::WriteDescriptorSet uniformWriteSet;
	uniformWriteSet.sType = vk::StructureType::eWriteDescriptorSet;
	uniformWriteSet.setDescriptorCount(1)
				   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
				   .setDstArrayElement(0)
				   .setDstBinding(0)
				   .setDstSet(m_DescriptorSet)
				   .setPBufferInfo(&m_UniformBuffer.m_Descriptor);

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
	m_Device.GetLogicDevice().updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Application::CreateImageTexture(const char* path)
{
	int width, height, channel;
	stbi_uc* pixels = stbi_load(path, &width, &height, &channel, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("read imagefile failed!");
	}
	vk::DeviceSize size = width * height * 4;
	m_MipmapLevels = std::floor(std::log2(std::max(width, height))) + 1;

	Buffer stagingBuffer;
	m_Device.CreateBuffer(vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &stagingBuffer, pixels);
	CreateImage(m_Image, m_ImageMemory, m_MipmapLevels, vk::SampleCountFlagBits::e1, vk::Extent2D(width, height), vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal);
	CreateImageView(m_Image, m_ImageView, m_MipmapLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	TransiationImageLayout(m_Image, vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor, m_MipmapLevels);
	CopyBufferToImage(stagingBuffer.m_Buffer, m_Image, vk::Extent3D(width, height, 1));
	/*TransiationImageLayout(m_Image, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);*/
	GenerateMipmaps(m_Image, width, height, m_MipmapLevels);
}

void Application::CreateImage(vk::Image& image, vk::DeviceMemory& memory, uint32_t mipLevels, vk::SampleCountFlagBits sampleCount, vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags usage, vk::ImageTiling tiling, vk::MemoryPropertyFlags memoryPropertyFlags)
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.sType = vk::StructureType::eImageCreateInfo;
	imageInfo.setArrayLayers(1)
			 .setExtent(vk::Extent3D(extent.width, extent.height, 1))
			 .setFormat(format)
			 .setImageType(vk::ImageType::e2D)
			 .setInitialLayout(vk::ImageLayout::eUndefined)
			 .setMipLevels(mipLevels)
			 .setSamples(sampleCount)
			 .setSharingMode(vk::SharingMode::eExclusive)
			 .setTiling(tiling)
			 .setUsage(usage);
	if (m_Device.GetLogicDevice().createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
	{
		throw std::runtime_error("image create failed!");
	}

	vk::MemoryRequirements requirement;
	m_Device.GetLogicDevice().getImageMemoryRequirements(image, &requirement);
	vk::MemoryAllocateInfo memoryInfo;
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(requirement.size)
			  .setMemoryTypeIndex(m_Device.FindMemoryType(requirement.memoryTypeBits, memoryPropertyFlags));
	if (m_Device.GetLogicDevice().allocateMemory(&memoryInfo, nullptr, &memory) != vk::Result::eSuccess)
	{
		throw std::runtime_error("imageMemory allocate failed!");
	}
	m_Device.GetLogicDevice().bindImageMemory(image, memory, 0);
}

void Application::CopyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, vk::Extent3D extent)
{
	auto command = m_Device.AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

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

	m_Device.FlushCommandBuffer(command, m_Device.GetGraphicQueue());
}

void Application::TransiationImageLayout(vk::Image image, vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess, vk::ImageLayout srcLayout, vk::PipelineStageFlags dstStage,  vk::AccessFlags dstAccess, vk::ImageLayout dstLayout, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	auto command = m_Device.AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

	vk::ImageSubresourceRange range;
	range.setAspectMask(aspectFlags)
		 .setBaseArrayLayer(0)
		 .setBaseMipLevel(0)
		 .setLayerCount(1)
		 .setLevelCount(mipLevels);
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

	m_Device.FlushCommandBuffer(command, m_Device.GetGraphicQueue());
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
			   .setMinLod(0.0f)
			   .setMaxLod(static_cast<float>(m_MipmapLevels))
			   .setUnnormalizedCoordinates(VK_FALSE);
	if (m_Device.GetLogicDevice().createSampler(&samplerInfo, nullptr, &m_Sampler) != vk::Result::eSuccess)
	{
		throw std::runtime_error("sampler create failed!");
	}
}

void Application::CreateDepthSources()
{
	vk::Format depthFormat = FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	
	CreateImage(m_DepthImage, m_DepthMemory, 1, m_SamplerCount, m_SwapChain.GetExtent(), depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal);
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
	if (HasStencil(depthFormat))
	{
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}
	CreateImageView(m_DepthImage, m_DepthImageView, 1, depthFormat, aspectFlags);
	TransiationImageLayout(m_DepthImage, vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, aspectFlags, 1);
}

void Application::CreateImageView(vk::Image image, vk::ImageView& imageView, uint32_t mipLevels, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageViewType viewType, vk::ComponentMapping mapping)
{
	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.setAspectMask(aspectFlags)
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(mipLevels);
	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	imageViewInfo.setComponents(mapping)
				 .setFormat(format)
				 .setImage(image)
				 .setViewType(viewType)
				 .setSubresourceRange(subresourceRange);
	if (m_Device.GetLogicDevice().createImageView(&imageViewInfo, nullptr, &imageView) != vk::Result::eSuccess)
	{
		throw std::runtime_error("ImageView Create failed!");
	}
}

vk::Format Application::FindImageFormatDeviceSupport(const std::vector<vk::Format> formats, vk::ImageTiling tiling, vk::FormatFeatureFlags featureFlags)
{
	for (auto& format : formats)
	{
		auto formatProperties = m_Device.GetPhysicalDevice().getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && ((formatProperties.linearTilingFeatures & featureFlags) == featureFlags))
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && ((formatProperties.optimalTilingFeatures & featureFlags) == featureFlags))
		{
			return format;
		}
	}
	throw std::runtime_error("image Format not found!");
}

bool Application::HasStencil(vk::Format format)
{
	if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
	{
		return true;
	}
	return false;
}

//void Application::ClearSwapchain()
//{
//	for (auto& frameBuffer : m_FrameBuffers)
//	{
//		m_Device.GetLogicDevice().destroyFramebuffer(frameBuffer);
//	}
//
//	for (auto& imageView : m_SwapChain.GetImageViews())
//	{
//		m_Device.GetLogicDevice().destroyImageView(imageView);
//	}
//
//	m_Device.GetLogicDevice().destroySwapchainKHR(m_SwapChain.GetSwapChain());
//
//	m_Device.GetLogicDevice().destroyImageView(m_DepthImageView);
//	m_Device.GetLogicDevice().destroyImage(m_DepthImage);
//	m_Device.GetLogicDevice().freeMemory(m_DepthMemory);
//
//	m_Device.GetLogicDevice().destroyImageView(m_ColorView);
//	m_Device.GetLogicDevice().destroyImage(m_ColorImage);
//	m_Device.GetLogicDevice().freeMemory(m_ColorMemory);
//}

//void Application::ReCreateSwapchain()
//{
//	int width, height;
//	m_Window.GetFrameBufferSize(&width, &height);
//	while (width ==0 || height == 0)
//	{
//		m_Window.GetFrameBufferSize(&width, &height);
//		glfwWaitEvents();
//	}
//	m_Device.GetLogicDevice().waitIdle();
//	ClearSwapchain();
//	//CreateSwapchain();
//	CreateColorSources();
//	CreateDepthSources();
//	CreateFrameBuffer();
//}

void Application::LoadModel(const char* path)
{
	tinyobj::attrib_t attris;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string error;
	bool res = tinyobj::LoadObj(&attris, &shapes, &materials, &warn, &error, path);
	if (!res)
	{
		throw std::runtime_error("load model failed!");
	}
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (auto& shape : shapes)
	{
		for (auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.Position = {
				attris.vertices[3 * index.vertex_index + 0],
				attris.vertices[3 * index.vertex_index + 1],
				attris.vertices[3 * index.vertex_index + 2]
			};

			vertex.Coord = {
				attris.texcoords[2 * index.texcoord_index + 0],
				1.0f - attris.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.Color = { 1.0f, 1.0f, 1.0f };
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(m_VertexData.size());
				m_VertexData.push_back(vertex);
			}
			m_Indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Application::GenerateMipmaps(vk::Image image, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels)
{
	uint32_t mipWidth = texWidth;
	uint32_t mipHeight = texHeight;

	auto command = m_Device.AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

	vk::ImageSubresourceRange region;
	region.setAspectMask(vk::ImageAspectFlagBits::eColor)
		  .setBaseArrayLayer(0)
		  .setLayerCount(1)
		  .setLevelCount(1);

	vk::ImageMemoryBarrier barrier;
	barrier.sType = vk::StructureType::eImageMemoryBarrier;
	barrier.setImage(image)
		   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

	for (size_t i = 1; i < mipLevels; i++)
	{
		region.setBaseMipLevel(i - 1);
		barrier.setSubresourceRange(region)
			   .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
			   .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			   .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
			   .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
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

		command.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
			   .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
			   .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			   .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);
		mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
		mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
	}

	region.setBaseMipLevel(mipLevels - 1);
	barrier.setSubresourceRange(region)
		   .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
		   .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
		   .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		   .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);
	
	m_Device.FlushCommandBuffer(command, m_Device.GetGraphicQueue());
}

void Application::CreateColorSources()
{
	CreateImage(m_ColorImage, m_ColorMemory, 1, m_SamplerCount, m_SwapChain.GetExtent(), m_SwapChain.GetFormat(), vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal);
	CreateImageView(m_ColorImage, m_ColorView, 1, m_SwapChain.GetFormat(), vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2D);
}