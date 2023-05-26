#include "Application.h"
#include <set>
#include <limits>
#include <chrono>
#include <unordered_map>

#include <gtc/matrix_transform.hpp>


#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>



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
	commandManager.SetContext(m_Device);
	m_SamplerCount = m_Device.GetMaxSampleCount();
	m_SwapChain.Init(m_Device, m_Window, m_SamplerCount, false, true, commandManager);
	CreateSetLayout();
	CreatePipeLine();

	CreateImageTexture("resource/textures/vikingRoom.png");
	LoadModel("resource/models/vikingRoom.obj");
	
	m_CommandBuffer = commandManager.AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary, true);
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateSampler(m_Texture.GetMipLevel());
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
				.setRenderPass(m_SwapChain.GetRenderPass())
				.setSubpass(0)
				.setPDynamicState(&dynamicState)
				.setBasePipelineHandle(VK_NULL_HANDLE)
				.setBasePipelineIndex(-1);
	if (m_Device.GetLogicDevice().createGraphicsPipelines({}, 1, &pipelineInfo, nullptr, &m_Pipeline) != vk::Result::eSuccess)
	{
		throw std::runtime_error("pipeline Create failed!");
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
					   .setFramebuffer(m_SwapChain.GetFrameBuffers()[imageIndex].GetVkFrameBuffer())
					   .setRenderPass(m_SwapChain.GetRenderPass())
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
			 .setImageView(m_Texture.GetImage().GetVkImageView())
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
	m_Texture.Create(m_Device, commandManager, "resource/textures/vikingRoom.png", true);
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