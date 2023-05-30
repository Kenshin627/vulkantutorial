#include "Core.h"
#include "Application.h"
#include <set>
#include <limits>
#include <chrono>
#include <unordered_map>
#include <gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "../vendor/tiny_obj_loader/tiny_obj_loader.h"

void Application::Run()
{
	InitContext();
	RenderLoop();
	Clear();
}

void Application::InitWindow(int width, int height, const char* title)
{
	m_Window = Window(width, height, title);
}

void Application::InitContext()
{
	m_Device = Device(m_Window);
	//m_SamplerCount = m_Device.GetMaxSampleCount();
	m_SamplerCount = vk::SampleCountFlagBits::e1;
	m_SwapChain.Init(m_Device, m_Window, m_SamplerCount, false, true);
	CreateRenderPass();
	m_Texture.Create(m_Device, "resource/textures/vikingRoom.png", true);
	m_CubeTexture.Create(m_Device, "resource/textures/nirvana.jpg", true);
	m_SkyBoxTexture.Create(m_Device, { "resource/textures/skybox/right.jpg", "resource/textures/skybox/left.jpg", "resource/textures/skybox/top.jpg", "resource/textures/skybox/bottom.jpg", "resource/textures/skybox/front.jpg", "resource/textures/skybox/back.jpg" });
	CreateUniformBuffer();

	CreateSetLayout();
	BuildAndUpdateDescriptorSets();
	CreatePipeLine();
	
	LoadModel("resource/models/vikingRoom.obj", m_VertexData, m_Indices);	
	LoadModel("resource/models/cube.obj", m_CubeVexData, m_CubeIndices);
	m_CommandBuffer = m_Device.GetCommandManager().AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary, true);

	CreateVertexBuffer();
	CreateIndexBuffer();
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
	Shader vertex(m_Device.GetLogicDevice(), "resource/shaders/pushConstantVert.spv");
	vertex.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eVertex);
	Shader fragment(m_Device.GetLogicDevice(), "resource/shaders/pushConstantFrag.spv");
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
					.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
					.setDepthWriteEnable(VK_TRUE)
					.setFront({})
					.setMaxDepthBounds(1.0f)
					.setMinDepthBounds(0.0f)
					.setStencilTestEnable(VK_FALSE);

	//7.
	vk::PipelineViewportStateCreateInfo viewportInfo;
	viewportInfo.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
	viewportInfo.setViewportCount(1)
		        .setScissorCount(1);

	//8.
	std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
	dynamicState.setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
		        .setPDynamicStates(dynamicStates.data());

	//9.
	vk::PipelineMultisampleStateCreateInfo multisamplesInfo;
	multisamplesInfo.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
	multisamplesInfo.setRasterizationSamples(m_SamplerCount)
			        .setMinSampleShading(0.2f)
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
				.setLayout(m_PushConstantSetlayout.GetPipelineLayout())
				.setRenderPass(DeferredRendingPass.GetVkRenderPass())
				.setSubpass(0)
				.setPDynamicState(&dynamicState)
				.setBasePipelineHandle(VK_NULL_HANDLE)
				.setBasePipelineIndex(-1)
				.setFlags(vk::PipelineCreateFlagBits::eAllowDerivatives);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createGraphicsPipelines({}, 1, &pipelineInfo, nullptr, &m_PipeLines.Phong));

	//GrayScale
	vk::PipelineVertexInputStateCreateInfo emptyInput;
	pipelineInfo.setFlags(vk::PipelineCreateFlagBits::eDerivative)
				.setBasePipelineHandle(m_PipeLines.Phong)
				.setBasePipelineIndex(-1)
				.setPVertexInputState(&emptyInput)
				.setSubpass(1)
				.setLayout(m_InputAttachmentSetlayouts[0].GetPipelineLayout());
	rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
	depthStencilInfo.setDepthWriteEnable(false);
	vertex = Shader(m_Device.GetLogicDevice(), "resource/shaders/grayscaleVert.spv");
	vertex.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eVertex);
	fragment = Shader(m_Device.GetLogicDevice(), "resource/shaders/grayscaleFrag.spv");
	fragment.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eFragment);
	shaders[0] = vertex.m_ShaderStage;
	shaders[1] = fragment.m_ShaderStage;
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createGraphicsPipelines({}, 1, & pipelineInfo, nullptr, & m_PipeLines.GrayScale));

	////skyBox
	//vertex = Shader(m_Device.GetLogicDevice(), "resource/shaders/skyboxVert.spv");
	//vertex.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eVertex);
	//fragment = Shader(m_Device.GetLogicDevice(), "resource/shaders/skyboxFrag.spv");
	//fragment.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eFragment);
	//shaders[0] = vertex.m_ShaderStage;
	//shaders[1] = fragment.m_ShaderStage;
	//rasterizationInfo.setCullMode(vk::CullModeFlagBits::eFront);
	//depthStencilInfo.setDepthWriteEnable(false).setDepthTestEnable(false);
	//VK_CHECK_RESULT(m_Device.GetLogicDevice().createGraphicsPipelines({}, 1, & pipelineInfo, nullptr, & m_PipeLines.SkyBox));
}

void Application::RecordCommandBuffer(vk::CommandBuffer command, uint32_t imageIndex)
{
	m_Device.GetCommandManager().CommandBegin(command);
		vk::Extent2D extent = m_SwapChain.GetExtent();
		DeferredRendingPass.Begin(command, imageIndex);
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
			command.setViewport(0, 1, &viewport);
			command.setScissor(0, 1, &scissor);
			vk::DeviceSize size(0);

			//subpass 0
			{
				auto DescriptorSet = m_PushConstantSetlayout.GetDescriptorSet();
				command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PushConstantSetlayout.GetPipelineLayout(), 0, 1, &DescriptorSet, 0, nullptr);
				command.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLines.Phong);
				command.bindVertexBuffers(0, 1, &m_CubeVertexBuffer.m_Buffer, &size);
				command.bindIndexBuffer(m_CubeIndexBuffer.m_Buffer, 0, vk::IndexType::eUint32);
				for (auto& cubeConst : CubePushConstants)
				{
					command.pushConstants(m_PushConstantSetlayout.GetPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConsntantCube), &cubeConst);
					command.drawIndexed(static_cast<uint32_t>(m_CubeIndices.size()), 1, 0, 0, 0);
				}
			}

			//subpass 1
			{
				auto DescriptorSet = m_InputAttachmentSetlayouts[imageIndex].GetDescriptorSet();
				command.nextSubpass(vk::SubpassContents::eInline);
				command.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLines.GrayScale);
				command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_InputAttachmentSetlayouts[imageIndex].GetPipelineLayout(), 0, 1, &DescriptorSet, 0, nullptr);
				command.draw(3, 1, 0, 0);
			}
		//command.endRenderPass();
			DeferredRendingPass.End(command);
	m_Device.GetCommandManager().CommandEnd(command);
}			

void Application::DrawFrame()
{

	auto fenceResult = m_Device.GetLogicDevice().waitForFences(1, &m_InFlightFence, VK_TRUE, (std::numeric_limits<uint64_t>::max)());

	uint32_t imageIndex;
	m_SwapChain.AcquireNextImage(&imageIndex, m_WaitAcquireImageSemaphore, this);
	
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

	m_SwapChain.PresentImage(imageIndex, m_WaitFinishDrawSemaphore, this);
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

	Buffer::CopyBuffer(stagingBuffer.m_Buffer, 0, m_VertexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
	//stagingBuffer.Clear();

	vk::DeviceSize cubeDatasize = sizeof(m_CubeVexData[0]) * m_CubeVexData.size();
	Buffer cubeStagingBuffer;
	cubeStagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, cubeDatasize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_CubeVexData.data());
	m_CubeVertexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	Buffer::CopyBuffer(cubeStagingBuffer.m_Buffer, 0, m_CubeVertexBuffer.m_Buffer, 0, cubeDatasize, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
}

void Application::CreateIndexBuffer()
{
	vk::DeviceSize size = sizeof(uint32_t) * m_Indices.size();
	Buffer stagingBuffer;
	stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Indices.data());
	m_IndexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	Buffer::CopyBuffer(stagingBuffer.m_Buffer, 0, m_IndexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
	//stagingBuffer.Clear();

	vk::DeviceSize cubeDataSize = sizeof(uint32_t) * m_CubeIndices.size();
	Buffer cubeStagingBuffer;
	cubeStagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, cubeDataSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_CubeIndices.data());
	m_CubeIndexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	Buffer::CopyBuffer(cubeStagingBuffer.m_Buffer, 0, m_CubeIndexBuffer.m_Buffer, 0, cubeDataSize, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
}

void Application::CreateSetLayout()
{
	m_PushConstantSetlayout.Create(m_Device, { 
		{ vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0, m_UniformBuffer.m_Descriptor, {}, 1},
		{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 1 , {}, m_CubeTexture.GetDescriptor(), 1}
	});
	m_SetCount++;
	m_PoolSizes.insert(m_PoolSizes.end(), m_PushConstantSetlayout.GetPoolSizes().begin(), m_PushConstantSetlayout.GetPoolSizes().end());

	m_InputAttachmentSetlayouts.resize(m_SwapChain.GetImageCount());
	for (uint32_t i = 0; i < m_SwapChain.GetImageCount(); i++)
	{
		m_InputAttachmentSetlayouts[i].Create(m_Device, {
			{ vk::DescriptorType::eInputAttachment, vk::ShaderStageFlagBits::eFragment, 0, {}, vk::DescriptorImageInfo({}, DeferredRendingPass.GetFrameBuffers()[i].GetAttachments()[1], vk::ImageLayout::eShaderReadOnlyOptimal)},
			{ vk::DescriptorType::eInputAttachment, vk::ShaderStageFlagBits::eFragment, 1, {}, vk::DescriptorImageInfo({}, DeferredRendingPass.GetFrameBuffers()[i].GetAttachments()[2], vk::ImageLayout::eShaderReadOnlyOptimal) }
		});
		m_SetCount++;
		m_PoolSizes.insert(m_PoolSizes.end(), m_InputAttachmentSetlayouts[i].GetPoolSizes().begin(), m_InputAttachmentSetlayouts[i].GetPoolSizes().end());
	}
}

void Application::BuildAndUpdateDescriptorSets()
{
	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
	descriptorPoolInfo.setMaxSets(m_SetCount)
		.setPoolSizeCount(static_cast<uint32_t>(m_PoolSizes.size()))
		.setPPoolSizes(m_PoolSizes.data());
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createDescriptorPool(&descriptorPoolInfo, nullptr, &m_DescriptorPool));

	m_PushConstantSetlayout.BuildAndUpdateSet(m_DescriptorPool);

	for (uint32_t i = 0; i < m_SwapChain.GetImageCount(); i++)
	{
		m_InputAttachmentSetlayouts[i].BuildAndUpdateSet(m_DescriptorPool);
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
	ubo.View = glm::translate(glm::mat4(1.0f), glm::vec3(0, -1.0, -4.5));
	ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_UniformBuffer.CopyFrom(&ubo, sizeof(UniformBufferObject));
}

void Application::LoadModel(const char* path, std::vector<Vertex>& vertexData, std::vector<uint32_t>& indicesData)
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
				uniqueVertices[vertex] = static_cast<uint32_t>(vertexData.size());
				vertexData.push_back(vertex);
			}
			indicesData.push_back(uniqueVertices[vertex]);
		}
	}
}

void Application::CreateRenderPass()
{
	vk::Format format = m_SwapChain.GetFormat();
	vk::AttachmentDescription presentAttachment;
	presentAttachment.setFormat(format)
					 .setSamples(vk::SampleCountFlagBits::e1)
					 .setInitialLayout(vk::ImageLayout::eUndefined)
					 .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
					 .setLoadOp(vk::AttachmentLoadOp::eClear)
					 .setStoreOp(vk::AttachmentStoreOp::eStore)
					 .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
					 .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription colorAttachment;
	colorAttachment.setFormat(format)
				   .setSamples(m_SamplerCount)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::Format depthFormat = m_Device.FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	vk::AttachmentDescription depthAttachment;
	depthAttachment.setFormat(depthFormat)
			       .setSamples(m_SamplerCount)
			       .setInitialLayout(vk::ImageLayout::eUndefined)
			       .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			       .setLoadOp(vk::AttachmentLoadOp::eClear)
			       .setStoreOp(vk::AttachmentStoreOp::eStore)
			       .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			       .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	std::vector<vk::AttachmentDescription> attachments = { presentAttachment, colorAttachment, depthAttachment };

	std::vector<vk::SubpassDescription> subpassDescs;

	//first subpass
	vk::AttachmentReference colorReference;
	colorReference.setAttachment(1).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(2).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::SubpassDescription subpass0;
	subpass0.setColorAttachmentCount(1)
		    .setPColorAttachments(&colorReference)
		    .setPDepthStencilAttachment(&depthReference)
		    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpassDescs.push_back(subpass0);

	//second subpass
	vk::AttachmentReference colorReferenceSwapchain;
	colorReferenceSwapchain.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference colorInput;
	colorInput.setAttachment(1).setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::AttachmentReference depthInput;
	depthInput.setAttachment(2).setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	std::array<vk::AttachmentReference, 2> inputAttachments = { colorInput, depthInput };

	vk::SubpassDescription subpass1;
	subpass1.setColorAttachmentCount(1)
		    .setPColorAttachments(&colorReferenceSwapchain)
		    .setInputAttachmentCount(static_cast<uint32_t>(inputAttachments.size()))
		    .setPInputAttachments(inputAttachments.data())
		    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpassDescs.push_back(subpass1);

	std::vector<vk::SubpassDependency> dependencies;
	vk::SubpassDependency d1, d2, d3;
	d1.setSrcSubpass(VK_SUBPASS_EXTERNAL)
	  .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
	  .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
	  .setDstSubpass(0)
	  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
	  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
	  .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

	d2.setSrcSubpass(0)
	  .setDstSubpass(1)
	  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	  .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
	  .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
	  .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	  .setDependencyFlags(vk::DependencyFlagBits::eByRegion);


	d3.setSrcSubpass(0)
	  .setDstSubpass(VK_SUBPASS_EXTERNAL)
	  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	  .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
	  .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
	  .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
	  .setDependencyFlags(vk::DependencyFlagBits::eByRegion);
	dependencies.push_back(d1);
	dependencies.push_back(d2);
	dependencies.push_back(d3);

	std::vector<vk::ClearValue> clearValues(3);
	clearValues[0].color = vk::ClearColorValue();
	clearValues[1].color = vk::ClearColorValue();
	clearValues[2].depthStencil = vk::ClearDepthStencilValue(1.0f, 1);

	vk::Extent2D extent = m_SwapChain.GetExtent();
	vk::Rect2D renderArea;
	renderArea.setOffset(vk::Offset2D(0, 0))
			  .setExtent(extent);

	DeferredRendingPass.Create(m_Device, attachments, subpassDescs, dependencies, clearValues, renderArea);


	DeferredRendingPass.BuildFrameBuffer(PrepareFrameBufferAttachmentsData(), extent.width, extent.height);
}

std::vector<std::vector<vk::ImageView>> Application::PrepareFrameBufferAttachmentsData()
{
	//build color and depth attachments
	vk::Format format = m_SwapChain.GetFormat();
	vk::Extent2D extent = m_SwapChain.GetExtent();
	uint32_t imageCount = m_SwapChain.GetImageCount();
	vk::Format depthFormat = m_Device.FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	vk::Extent3D size(extent.width, extent.height, 1);
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
	if (m_Device.HasStencil(depthFormat))
	{
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}

	std::vector<std::vector<vk::ImageView>> bufferAttachments;
	bufferAttachments.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		//0 presentKHR
		std::vector<vk::ImageView> attachments;
		attachments.push_back(m_SwapChain.GetSwapChainImageViews()[i].GetVkImageView());

		//1 color Attachment
		Image colorAttachment;
		colorAttachment.Create(m_Device, 1, m_SamplerCount, vk::ImageType::e2D, size, format, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
		colorAttachment.CreateImageView(format);
		attachments.push_back(colorAttachment.GetVkImageView());

		//2 depth Attachment
		Image depthAttachment;
		depthAttachment.Create(m_Device, 1, m_SamplerCount, vk::ImageType::e2D, size, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});

		depthAttachment.CreateImageView(depthFormat, aspectFlags);

		//TODO remove
		depthAttachment.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, aspectFlags);
		attachments.push_back(depthAttachment.GetVkImageView());
		bufferAttachments[i] = attachments;
	}
	return bufferAttachments;
}