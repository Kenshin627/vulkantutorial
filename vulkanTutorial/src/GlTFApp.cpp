#include "Core.h"
#include "GlTFApp.h"
#include <set>
#include <limits>
#include <chrono>
#include <unordered_map>
#include <gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "../vendor/tiny_obj_loader/tiny_obj_loader.h"

void GLTFApp::Run()
{
	InitContext();
	RenderLoop();
	Clear();
}

void GLTFApp::InitWindow(int width, int height, const char* title)
{
	m_Window = Window(width, height, title);
}

void GLTFApp::InitContext()
{
	//m_SamplerCount = m_Device.GetMaxSampleCount();
	m_Device = Device(m_Window);
	m_SamplerCount = vk::SampleCountFlagBits::e1;
	m_SwapChain.Init(m_Device, m_Window, m_SamplerCount, false, true);
	CreateRenderPass();

	m_SkyBoxTexture.Create(m_Device, { "resource/textures/skybox/right.jpg", "resource/textures/skybox/left.jpg", "resource/textures/skybox/top.jpg", "resource/textures/skybox/bottom.jpg", "resource/textures/skybox/front.jpg", "resource/textures/skybox/back.jpg" });
	m_Model.LoadModel(m_Device, "resource/models/FlightHelmet/glTF/FlightHelmet.gltf");

	CreateUniformBuffer();
	CreateSetLayout();
	BuildAndUpdateDescriptorSets();
	CreatePipeLine();

	LoadModel("resource/models/cube.obj", m_SkyboxVexData, m_SkyboxIndices);
	m_CommandBuffer = m_Device.GetCommandManager().AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary, true);

	
	CreateAsyncObjects();
}

void GLTFApp::RenderLoop()
{
	while (!m_Window.ShouldClose())
	{
		m_Window.PollEvents();
		DrawFrame();
	}
	m_Device.GetLogicDevice().waitIdle();
}

void GLTFApp::Clear()
{

}

void GLTFApp::CreatePipeLine()
{
	vk::PipelineVertexInputStateCreateInfo vertexInput;
	vertexInput.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
	auto bindingDesc = GlTFModel::Vertex::GetBindingDescriptions();
	auto attributeDesc = GlTFModel::Vertex::GetAttributeDescriptions();
	vertexInput.setVertexBindingDescriptionCount(static_cast<uint32_t>(bindingDesc.size())).setPVertexBindingDescriptions(bindingDesc.data())
			   .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDesc.size())).setPVertexAttributeDescriptions(attributeDesc.data());

	//2.
	vk::PipelineInputAssemblyStateCreateInfo assemblyInfo;
	assemblyInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
	assemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList)
			    .setPrimitiveRestartEnable(VK_FALSE);

	//3.
	Shader vertex(m_Device.GetLogicDevice(), "resource/shaders/meshVert.spv");
	vertex.SetPipelineShaderStageInfo(vk::ShaderStageFlagBits::eVertex);
	Shader fragment(m_Device.GetLogicDevice(), "resource/shaders/meshFrag.spv");
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
				.setLayout(BlinnPhongLayout)
				.setRenderPass(BlinnPhongPass.GetVkRenderPass())
				.setSubpass(0)
				.setPDynamicState(&dynamicState)
				.setBasePipelineHandle(VK_NULL_HANDLE)
				.setBasePipelineIndex(-1);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createGraphicsPipelines({}, 1, &pipelineInfo, nullptr, &m_PipeLines.BlinnPhong));
}

void GLTFApp::RecordCommandBuffer(vk::CommandBuffer command, uint32_t imageIndex)
{
	m_Device.GetCommandManager().CommandBegin(command);
	vk::Extent2D extent = m_SwapChain.GetExtent();
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
	vk::DeviceSize size(0);
	{
		BlinnPhongPass.Begin(command, imageIndex, vk::Rect2D({ 0,0 }, extent));
		command.setViewport(0, 1, &viewport);
		command.setScissor(0, 1, &scissor);
		command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, BlinnPhongLayout, 0, 1, &MatricesSet, 0, nullptr);
		command.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLines.BlinnPhong);
		m_Model.Draw(command, BlinnPhongLayout);
		BlinnPhongPass.End(command);
	}
	m_Device.GetCommandManager().CommandEnd(command);
}

void GLTFApp::DrawFrame()
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

void GLTFApp::CreateAsyncObjects()
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

void GLTFApp::CreateVertexBuffer()
{
	//vk::DeviceSize size = sizeof(m_VertexData[0]) * m_VertexData.size();
	//Buffer stagingBuffer;
	//stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_VertexData.data());

	//m_VertexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);

	//Buffer::CopyBuffer(stagingBuffer.m_Buffer, 0, m_VertexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
	////stagingBuffer.Clear();

	//vk::DeviceSize cubeDatasize = sizeof(m_CubeVexData[0]) * m_CubeVexData.size();
	//Buffer cubeStagingBuffer;
	//cubeStagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, cubeDatasize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_CubeVexData.data());
	//m_CubeVertexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	//Buffer::CopyBuffer(cubeStagingBuffer.m_Buffer, 0, m_CubeVertexBuffer.m_Buffer, 0, cubeDatasize, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
}

void GLTFApp::CreateIndexBuffer()
{
	//vk::DeviceSize size = sizeof(uint32_t) * m_Indices.size();
	//Buffer stagingBuffer;
	//stagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Indices.data());
	//m_IndexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	//Buffer::CopyBuffer(stagingBuffer.m_Buffer, 0, m_IndexBuffer.m_Buffer, 0, size, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
	////stagingBuffer.Clear();

	//vk::DeviceSize cubeDataSize = sizeof(uint32_t) * m_CubeIndices.size();
	//Buffer cubeStagingBuffer;
	//cubeStagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, cubeDataSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_CubeIndices.data());
	//m_CubeIndexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	//Buffer::CopyBuffer(cubeStagingBuffer.m_Buffer, 0, m_CubeIndexBuffer.m_Buffer, 0, cubeDataSize, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());
}

void GLTFApp::CreateSetLayout()
{
	//BlinnPhongSetLayout.Create(m_Device, {
	//	{ vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0, m_UniformBuffer.m_Descriptor, {}, 1 },
	//	{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 1, {}, m_CubeTexture.GetDescriptor(), 1 }
	//	});
	//m_SetCount++;
	//m_PoolSizes.insert(m_PoolSizes.end(), SetLayout1.GetPoolSizes().begin(), SetLayout1.GetPoolSizes().end());


	//SetLayout2.Create(m_Device, {
	//	{vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 0, {}, renderpass1.GetFrameBuffers()[0].GetColorAttachment(0).Attachment.GetDescriptor(), 1}
	//	});
	//m_SetCount++;
	//m_PoolSizes.insert(m_PoolSizes.end(), SetLayout2.GetPoolSizes().begin(), SetLayout2.GetPoolSizes().end());
}

void GLTFApp::BuildAndUpdateDescriptorSets()
{
	auto vkDevice = m_Device.GetLogicDevice();
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, m_Model.GetTextureCount())
	};
	const uint32_t maxSetCount = static_cast<uint32_t>(m_Model.GetTextureCount() + 1);
	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
	poolInfo.setMaxSets(maxSetCount)
			.setPoolSizeCount(poolSizes.size())
			.setPPoolSizes(poolSizes.data());
	vk::DescriptorPool pool;
	VK_CHECK_RESULT(vkDevice.createDescriptorPool(&poolInfo, nullptr, &pool));

	{
		vk::DescriptorSetLayoutBinding setLayoutBinding;
		setLayoutBinding.setBinding(0)
						.setDescriptorCount(1)
						.setDescriptorType(vk::DescriptorType::eUniformBuffer)
						.setStageFlags(vk::ShaderStageFlagBits::eVertex);
		vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
		setLayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
		setLayoutInfo.setBindingCount(1)
					 .setPBindings(&setLayoutBinding);
		VK_CHECK_RESULT(vkDevice.createDescriptorSetLayout(&setLayoutInfo, nullptr, &SetLayouts.matrices));
	}
	
	 //textures
	{	
		vk::DescriptorSetLayoutBinding setLayoutBinding;
		setLayoutBinding.setBinding(0)
						.setDescriptorCount(1)
						.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
						.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
		setLayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
		setLayoutInfo.setBindingCount(1)
					 .setPBindings(&setLayoutBinding);
		VK_CHECK_RESULT(vkDevice.createDescriptorSetLayout(&setLayoutInfo, nullptr, &SetLayouts.textures));
	}

	vk::PushConstantRange pushConst;
	pushConst.setOffset(0)
			 .setSize(sizeof(glm::mat4))
			 .setStageFlags(vk::ShaderStageFlagBits::eVertex);

	std::array<vk::DescriptorSetLayout, 2> setLayouts = { SetLayouts.matrices, SetLayouts.textures };
	vk::PipelineLayoutCreateInfo pipelineInfo;
	pipelineInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	pipelineInfo.setSetLayoutCount(setLayouts.size())
				.setPSetLayouts(setLayouts.data())
				.setPushConstantRangeCount(1)
				.setPPushConstantRanges(&pushConst);
	VK_CHECK_RESULT(vkDevice.createPipelineLayout(&pipelineInfo, nullptr, &BlinnPhongLayout));

	vk::DescriptorSetAllocateInfo allocInf;
	allocInf.sType = vk::StructureType::eDescriptorSetAllocateInfo;
	allocInf.setDescriptorPool(pool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&SetLayouts.matrices);
	VK_CHECK_RESULT(vkDevice.allocateDescriptorSets(&allocInf, &MatricesSet));
	vk::WriteDescriptorSet writeSet;
	writeSet.sType = vk::StructureType::eWriteDescriptorSet;
	writeSet.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDstArrayElement(0)
			.setDstBinding(0)
			.setDstSet(MatricesSet)
			.setPBufferInfo(&m_UniformBuffer.m_Descriptor);
	vkDevice.updateDescriptorSets(1, &writeSet, 0, nullptr);

	for (auto& image : m_Model.GetImageSet())
	{
		auto imageDesc = image.Texture.GetDescriptor();
		vk::DescriptorSetAllocateInfo allocInf;
		allocInf.sType = vk::StructureType::eDescriptorSetAllocateInfo;
		allocInf.setDescriptorPool(pool)
				.setDescriptorSetCount(1)
				.setPSetLayouts(&SetLayouts.textures);
		VK_CHECK_RESULT(vkDevice.allocateDescriptorSets(&allocInf, &image.DescSet));
		vk::WriteDescriptorSet writeSet;
		writeSet.sType = vk::StructureType::eWriteDescriptorSet;
		writeSet.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstArrayElement(0)
				.setDstBinding(0)
				.setDstSet(image.DescSet)
				.setPImageInfo(&imageDesc);
		vkDevice.updateDescriptorSets(1, &writeSet, 0, nullptr);
	}
}

void GLTFApp::CreateUniformBuffer()
{
	vk::DeviceSize size = sizeof(UniformBufferObject);
	m_UniformBuffer.Create(m_Device, vk::BufferUsageFlagBits::eUniformBuffer, size, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, nullptr);
	m_UniformBuffer.Map();
}

void GLTFApp::UpdateUniformBuffers()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.Proj = glm::perspective(glm::radians(45.0f), m_SwapChain.GetExtent().width / (float)m_SwapChain.GetExtent().height, 0.1f, 100.0f);
	ubo.Proj[1][1] *= -1;
	ubo.View = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.0, -1));
	ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_UniformBuffer.CopyFrom(&ubo, sizeof(UniformBufferObject));
}

void GLTFApp::LoadModel(const char* path, std::vector<Vertex>& vertexData, std::vector<uint32_t>& indicesData)
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

void GLTFApp::CreateRenderPass()
{
	vk::Format colorFormat = m_SwapChain.GetFormat();
	vk::Format depthFormat = m_Device.FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	uint32_t imageCount = m_SwapChain.GetImageCount();
	vk::Extent2D extent = m_SwapChain.GetExtent();
	vk::Rect2D renderArea;
	renderArea.setOffset(vk::Offset2D(0, 0))
			  .setExtent(extent);
	vk::Extent3D size(extent.width, extent.height, 1);
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
	if (m_Device.HasStencil(depthFormat))
	{
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}

	/*vk::AttachmentDescription colorAttachment;
	colorAttachment.setFormat(colorFormat)
				   .setSamples(m_SamplerCount)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);*/

	vk::AttachmentDescription depthAttachment;
	depthAttachment.setFormat(depthFormat)
				   .setSamples(m_SamplerCount)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eDontCare)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription colorResolve;
	colorResolve.setFormat(colorFormat)
			    .setSamples(vk::SampleCountFlagBits::e1)
			    .setInitialLayout(vk::ImageLayout::eUndefined)
			    .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
			    .setLoadOp(vk::AttachmentLoadOp::eClear)
			    .setStoreOp(vk::AttachmentStoreOp::eStore)
			    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	std::vector<vk::AttachmentDescription> attachments = { colorResolve, depthAttachment  };
	vk::AttachmentReference colorReference;
	colorReference.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	//vk::AttachmentReference resolveReference;
	//resolveReference.setAttachment(2).setLayout(vk::ImageLayout::eColorAttachmentOptimal);


	vk::SubpassDescription subpass;
	subpass.setColorAttachmentCount(1)
		   .setPColorAttachments(&colorReference)
		   .setPDepthStencilAttachment(&depthReference)
		   //.setPResolveAttachments(&resolveReference)
		   .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	std::vector<vk::SubpassDescription> subpasses = { subpass };

	std::vector<vk::SubpassDependency> subPassDependencies;
	subPassDependencies.resize(1);
	subPassDependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
					      .setDstSubpass(0)
					      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
					      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
					      .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
					      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

	std::vector<vk::ClearValue> clearValues(2);
	clearValues[0].color = vk::ClearColorValue();
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 1);

	BlinnPhongPass.Create(m_Device, attachments, subpasses, subPassDependencies, clearValues, renderArea);


	//Image colorImage;
	//colorImage.Create(m_Device, 1, m_SamplerCount, vk::ImageType::e2D, size, colorFormat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
	//colorImage.CreateImageView(colorFormat);

	Image depthImage;
	depthImage.Create(m_Device, 1, m_SamplerCount, vk::ImageType::e2D, size, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
	depthImage.CreateImageView(depthFormat, aspectFlags);

	//TODO remove
	depthImage.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, aspectFlags);

	std::vector<std::vector<FrameBufferAttachment>> bufferAttachments;
	for (auto& image : m_SwapChain.GetImages())
	{
		bufferAttachments.push_back({
			{ FrameBufferAttachment::AttachmentType::Color, image },
			//{ FrameBufferAttachment::AttachmentType::Color, colorImage },
			{ FrameBufferAttachment::AttachmentType::Depth, depthImage }
		});
	}
	BlinnPhongPass.BuildFrameBuffer(bufferAttachments, extent.width, extent.height);
}

void GLTFApp::RebuildFrameBuffer()
{
	vk::Format colorFormat = m_SwapChain.GetFormat();
	vk::Format depthFormat = m_Device.FindImageFormatDeviceSupport({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	uint32_t imageCount = m_SwapChain.GetImageCount();
	vk::Extent2D extent = m_SwapChain.GetExtent();
	vk::Rect2D renderArea;
	renderArea.setOffset(vk::Offset2D(0, 0))
		.setExtent(extent);
	vk::Extent3D size(extent.width, extent.height, 1);
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
	if (m_Device.HasStencil(depthFormat))
	{
		aspectFlags |= vk::ImageAspectFlagBits::eStencil;
	}
	Image colorImage;
	colorImage.Create(m_Device, 1, m_SamplerCount, vk::ImageType::e2D, size, colorFormat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
	colorImage.CreateImageView(colorFormat);

	//2 depth Attachment
	Image depthImage;
	depthImage.Create(m_Device, 1, m_SamplerCount, vk::ImageType::e2D, size, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageLayout::eUndefined, vk::SharingMode::eExclusive, 1, {});
	depthImage.CreateImageView(depthFormat, aspectFlags);


	//TODO remove
	depthImage.TransiationLayout(vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal, aspectFlags);

	std::vector<std::vector<FrameBufferAttachment>> bufferAttachments;
	for (auto& image : m_SwapChain.GetImages())
	{
		bufferAttachments.push_back({ 
			{ FrameBufferAttachment::AttachmentType::Color, colorImage },
			{ FrameBufferAttachment::AttachmentType::Depth, depthImage },
			{ FrameBufferAttachment::AttachmentType::Color, image      } ,
		});
	}
	BlinnPhongPass.ReBuildFrameBuffer(bufferAttachments, extent.width, extent.height);
}