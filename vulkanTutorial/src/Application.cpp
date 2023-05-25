#include "Application.h"
#include <set>
#include <limits>
#include <chrono>
#include <unordered_map>

#include <gtc/matrix_transform.hpp>


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
	commandManager.SetContext(m_Device);
	CreateColorSources();
	CreateDepthSources();
	CreateRenderPass();
	CreateFrameBuffer();
	CreateSetLayout();
	CreatePipeLine();

	CreateImageTexture("resource/textures/vikingRoom.png");
	LoadModel("resource/models/vikingRoom.obj");
	
	m_CommandBuffer = commandManager.AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary, false);
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateSampler(m_Texture.GetMiplevel());
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
	Shader vertex(m_Device.GetLogicDevice(), "resource/shaders/vert.spv");
	vertex.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eVertex);
	Shader fragment(m_Device.GetLogicDevice(), "resource/shaders/frag.spv");
	fragment.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eFragment);
	vk::PipelineShaderStageCreateInfo shaders[] = { vertex.m_ShaderStage, fragment.m_ShaderStage };

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

void Application::CreateFrameBuffer()
{
	vk::Device device = m_Device.GetLogicDevice();
	uint32_t width = m_SwapChain.GetExtent().width;
	uint32_t height = m_SwapChain.GetExtent().height;
	m_FrameBuffers.resize(m_SwapChain.GetImageViews().size());
	uint32_t index = 0;
	for (auto& view : m_SwapChain.GetImageViews())
	{
		m_FrameBuffers[index].SetAttachment(m_ColorImageView.GetVkImageView())
							 .SetAttachment(m_DepthImageView.GetVkImageView())
							 .SetAttachment(view);
		m_FrameBuffers[index++].Create(device, width, height, m_RenderPass);
	}
}

void Application::RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex)
{
	//vk::CommandBufferBeginInfo commandBufferBegin;
	//commandBufferBegin.sType = vk::StructureType::eCommandBufferBeginInfo;
	//commandBufferBegin.setPInheritanceInfo(nullptr);
	//				  //.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	//auto beginRes = buffer.begin(&commandBufferBegin);
	commandManager.CommandBegin(buffer);
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
					   .setFramebuffer(m_FrameBuffers[imageIndex].GetVkFrameBuffer())
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
	commandManager.CommandEnd(buffer);
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
	stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_VertexData.data());

	m_VertexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);

	Buffer::CopyBuffer(stagingBuffer.m_Buffer, 0, m_VertexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue(), commandManager);
	//stagingBuffer.Clear();
}

void Application::CreateIndexBuffer()
{
	vk::DeviceSize size = sizeof(uint32_t) * m_Indices.size();
	Buffer stagingBuffer;
	stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Indices.data());
	m_IndexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	Buffer::CopyBuffer(stagingBuffer.m_Buffer, 0, m_IndexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue(), commandManager);
	//stagingBuffer.Clear();
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
	m_UniformBuffer.Create(m_Device, vk::BufferUsageFlagBits::eUniformBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, nullptr);
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
			 .setImageView(m_TextureImageView.GetVkImageView())
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
	uint32_t mipLevel = std::floor(std::log2(std::max(width, height))) + 1;

	Buffer stagingBuffer;
	stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pixels);

	m_Texture.Create(m_Device, commandManager, mipLevel, vk::SampleCountFlagBits::e1, vk::ImageType::e2D, vk::Extent3D(width, height, 1), vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive);
	m_TextureImageView.Create(m_Device.GetLogicDevice(), m_Texture.GetVkImage(), vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevel);

	m_Texture.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);

	m_Texture.CopyBufferToImage(stagingBuffer.m_Buffer, vk::Extent3D(width, height, 1), vk::ImageLayout::eShaderReadOnlyOptimal);

	m_Texture.GenerateMipMaps();
	//stagingBuffer.Clear();
}

void Application::CreateSampler(uint32_t mipLevel)
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
			   .setMaxLod(static_cast<float>(mipLevel))
			   .setUnnormalizedCoordinates(VK_FALSE);
	if (m_Device.GetLogicDevice().createSampler(&samplerInfo, nullptr, &m_Sampler) != vk::Result::eSuccess)
	{
		throw std::runtime_error("sampler create failed!");
	}
}

void Application::CreateDepthSources()
{
	vk::Format depthFormat = FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	
	vk::Extent3D size(m_SwapChain.GetExtent().width, m_SwapChain.GetExtent().height, 1);
	m_DepthImage.Create(m_Device, commandManager, 1, m_SamplerCount, vk::ImageType::e2D, size, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive);
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
	if (HasStencil(depthFormat))
	{
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}
	m_DepthImageView.Create(m_Device.GetLogicDevice(), m_DepthImage.GetVkImage(), depthFormat, aspectFlags, 1, vk::ImageViewType::e2D);

	//TODO remove
	m_DepthImage.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, aspectFlags);
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

void Application::CreateColorSources()
{
	vk::Extent3D size(m_SwapChain.GetExtent().width, m_SwapChain.GetExtent().height, 1);
	vk::Format format = m_SwapChain.GetFormat();
	m_ColorImage.Create(m_Device, commandManager, 1, m_SamplerCount, vk::ImageType::e2D, size, format, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive);
	m_ColorImageView.Create(m_Device.GetLogicDevice(), m_ColorImage.GetVkImage(), format, vk::ImageAspectFlagBits::eColor, 1, vk::ImageViewType::e2D);
}