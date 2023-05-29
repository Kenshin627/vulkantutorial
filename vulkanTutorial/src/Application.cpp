#include "Core.h"
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
	m_Device = Device(window);
}

void Application::InitVulkan()
{
	InitDevice(m_Window);
	//m_SamplerCount = m_Device.GetMaxSampleCount();
	m_SamplerCount = vk::SampleCountFlagBits::e1;
	m_SwapChain.Init(m_Device, m_Window, m_SamplerCount, false, true);
	CreateSetLayout();
	CreatePipeLine();
	m_Texture.Create(m_Device, "resource/textures/vikingRoom.png", true);
	m_CubeTexture.Create(m_Device, "resource/textures/cube.jpg", true);
	m_SkyBoxTexture.Create(m_Device, { "resource/textures/skybox/right.jpg", "resource/textures/skybox/left.jpg", "resource/textures/skybox/top.jpg", "resource/textures/skybox/bottom.jpg", "resource/textures/skybox/front.jpg", "resource/textures/skybox/back.jpg" });
	LoadModel("resource/models/vikingRoom.obj", m_VertexData, m_Indices);	
	LoadModel("resource/models/cube.obj", m_CubeVexData, m_CubeIndices);
	m_CommandBuffer = m_Device.GetCommandManager().AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary, true);
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
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
				.setRenderPass(m_SwapChain.GetRenderPass())
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
				.setLayout(m_InputAttachmentSetlayout.GetPipelineLayout());
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
		std::array<vk::ClearValue, 3> clearValues{};
		clearValues[0].color = vk::ClearColorValue();
		clearValues[1].color = vk::ClearColorValue();
		clearValues[2].depthStencil = vk::ClearDepthStencilValue(1.0f, 1);
		
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

		command.beginRenderPass(&renderPassBegin, vk::SubpassContents::eInline);
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
				command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PushConstantSetlayout.GetPipelineLayout(), 0, 1, &m_DescriptorSet, 0, nullptr);
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
				command.nextSubpass(vk::SubpassContents::eInline);
				command.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLines.GrayScale);
				command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_InputAttachmentSetlayout.GetPipelineLayout(), 0, 1, &m_GrayScaleDescriptorSets[imageIndex], 0, nullptr);
				command.draw(3, 1, 0, 0);
			}
		command.endRenderPass();
	m_Device.GetCommandManager().CommandEnd(command);
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
	////firstPass set
	//vk::DescriptorSetLayoutBinding uniformBinding;
	//uniformBinding.setBinding(0)
	//	          .setDescriptorCount(1)
	//	          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
	//	          .setPImmutableSamplers(nullptr)
	//	          .setStageFlags(vk::ShaderStageFlagBits::eVertex);

	//vk::DescriptorSetLayoutBinding samplerBinding;
	//samplerBinding.setBinding(1)
	//			  .setDescriptorCount(1)
	//			  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	//			  .setPImmutableSamplers(nullptr)
	//			  .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	//std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uniformBinding, samplerBinding };

	//vk::DescriptorSetLayoutCreateInfo setlayoutInfo;
	//setlayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
	//setlayoutInfo.setBindingCount(static_cast<uint32_t>(bindings.size()))
	//			 .setPBindings(bindings.data());
	//VK_CHECK_RESULT(m_Device.GetLogicDevice().createDescriptorSetLayout(&setlayoutInfo, nullptr, &m_SetLayout));

	//vk::PipelineLayoutCreateInfo layoutInfo;
	//layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	//layoutInfo.setSetLayoutCount(1)
	//	      .setPSetLayouts(&m_SetLayout)
	//	      .setPushConstantRangeCount(0)
	//	      .setPPushConstantRanges(nullptr);
	//VK_CHECK_RESULT(m_Device.GetLogicDevice().createPipelineLayout(&layoutInfo, nullptr, &m_Layout));
	m_PushConstantSetlayout.Create(m_Device, { 
		{vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0 },
		{vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 1} 
	});

	m_InputAttachmentSetlayout.Create(m_Device, {
		{ vk::DescriptorType::eInputAttachment, vk::ShaderStageFlagBits::eFragment, 0 },
		{ vk::DescriptorType::eInputAttachment, vk::ShaderStageFlagBits::eFragment, 1 }
	});
	////secondpass set
	//vk::DescriptorSetLayoutBinding inputColotAttachment;
	//inputColotAttachment.setBinding(0)
	//					.setDescriptorCount(1)
	//					.setDescriptorType(vk::DescriptorType::eInputAttachment)
	//					.setPImmutableSamplers(nullptr)
	//					.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	//vk::DescriptorSetLayoutBinding inputDepthAttachment;
	//inputDepthAttachment.setBinding(1)
	//					.setDescriptorCount(1)
	//					.setDescriptorType(vk::DescriptorType::eInputAttachment)
	//					.setPImmutableSamplers(nullptr)
	//					.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	//std::array<vk::DescriptorSetLayoutBinding, 2> bindings2 = { inputColotAttachment, inputDepthAttachment };
	//vk::DescriptorSetLayoutCreateInfo subpass2SetlayoutInfo;
	//subpass2SetlayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
	//subpass2SetlayoutInfo.setBindingCount(static_cast<uint32_t>(bindings2.size()))
	//				     .setPBindings(bindings2.data());
	//VK_CHECK_RESULT(m_Device.GetLogicDevice().createDescriptorSetLayout(&subpass2SetlayoutInfo, nullptr, &m_GrayScaleSetLayout));
	//
	//vk::PipelineLayoutCreateInfo layoutInfo2;
	//layoutInfo2.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	//layoutInfo2.setSetLayoutCount(1)
	//		   .setPSetLayouts(&m_GrayScaleSetLayout)
	//		   .setPushConstantRangeCount(0)
	//		   .setPPushConstantRanges(nullptr);
	//VK_CHECK_RESULT(m_Device.GetLogicDevice().createPipelineLayout(&layoutInfo2, nullptr, &m_GrayScaleLayout));
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

void Application::CreateDescriptorPool()
{
	uint32_t imageCount = m_SwapChain.GetImageCount();
	vk::DescriptorPoolSize uniformPoolSize;
	uniformPoolSize.setDescriptorCount(1)
				   .setType(vk::DescriptorType::eUniformBuffer);

	vk::DescriptorPoolSize samplerPoolSize;
	samplerPoolSize.setDescriptorCount(1)
				   .setType(vk::DescriptorType::eCombinedImageSampler);

	/*vk::DescriptorPoolSize skyBoxSamplerPoolSize;
	skyBoxSamplerPoolSize.setDescriptorCount(1).setType(vk::DescriptorType::eCombinedImageSampler);*/

	vk::DescriptorPoolSize inputAttachmentPoolSize;
	inputAttachmentPoolSize.setDescriptorCount(imageCount * 2).setType(vk::DescriptorType::eInputAttachment);

	std::array<vk::DescriptorPoolSize, 3> poolSize = { uniformPoolSize, samplerPoolSize, inputAttachmentPoolSize };

	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
	descriptorPoolInfo.setMaxSets(imageCount + 1)
					  .setPoolSizeCount(static_cast<uint32_t>(poolSize.size()))
					  .setPPoolSizes(poolSize.data());
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createDescriptorPool(&descriptorPoolInfo, nullptr, &m_DescriptorPool));
}

void Application::CreateDescriptorSet()
{
	auto setLayout1 = m_PushConstantSetlayout.GetSetLayout();
	vk::DescriptorSetAllocateInfo setInfo;
	setInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
	setInfo.setDescriptorPool(m_DescriptorPool)
		   .setDescriptorSetCount(1)
		   .setPSetLayouts(&setLayout1);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().allocateDescriptorSets(&setInfo, &m_DescriptorSet));

	auto setLayout2 = m_InputAttachmentSetlayout.GetSetLayout();
	vk::DescriptorSetAllocateInfo setInfo2;
	setInfo2.sType = vk::StructureType::eDescriptorSetAllocateInfo;
	setInfo2.setDescriptorPool(m_DescriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&setLayout2);
	m_GrayScaleDescriptorSets.resize(m_SwapChain.GetImageCount());
	for (uint32_t i = 0; i < m_GrayScaleDescriptorSets.size(); i++)
	{
		VK_CHECK_RESULT(m_Device.GetLogicDevice().allocateDescriptorSets(&setInfo2, &m_GrayScaleDescriptorSets[i]));
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

	vk::DescriptorImageInfo imageDescriptor = m_CubeTexture.GetDescriptor();
	vk::WriteDescriptorSet samplerWrite;
	samplerWrite.sType = vk::StructureType::eWriteDescriptorSet;
	samplerWrite.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstArrayElement(0)
				.setDstBinding(1)
				.setDstSet(m_DescriptorSet)
				.setPImageInfo(&imageDescriptor);

	//vk::DescriptorImageInfo skyBoxSamplerDescriptor = m_SkyBoxTexture.GetDescriptor();
	//vk::WriteDescriptorSet skyboxSamplerWrite;
	//skyboxSamplerWrite.sType = vk::StructureType::eWriteDescriptorSet;
	//skyboxSamplerWrite.setDescriptorCount(1)
	//				  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	//				  .setDstArrayElement(0)
	//				  .setDstBinding(2)
	//				  .setDstSet(m_DescriptorSet)
	//				  .setPImageInfo(&skyBoxSamplerDescriptor);

	std::array<vk::WriteDescriptorSet, 2> writes = { uniformWriteSet, samplerWrite };
	m_Device.GetLogicDevice().updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

	//subpass2
	for (uint32_t i = 0; i < m_SwapChain.GetImageCount(); i++)
	{
		vk::DescriptorImageInfo colorInfo;
		colorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(m_SwapChain.GetFrameBuffers()[i].GetAttachments()[1]);
		vk::WriteDescriptorSet colorInput;
		colorInput.sType = vk::StructureType::eWriteDescriptorSet;
		colorInput.setDescriptorCount(1)
			.setDstArrayElement(0)
			.setDstBinding(0)
			.setDstSet(m_GrayScaleDescriptorSets[i])
			.setPImageInfo(&colorInfo)
			.setDescriptorType(vk::DescriptorType::eInputAttachment);

		vk::DescriptorImageInfo depthInfo;
		depthInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(m_SwapChain.GetFrameBuffers()[i].GetAttachments()[2]);
		vk::WriteDescriptorSet depthInput;
		depthInput.sType = vk::StructureType::eWriteDescriptorSet;
		depthInput.setDescriptorCount(1)
			.setDstArrayElement(0)
			.setDstBinding(1)
			.setDstSet(m_GrayScaleDescriptorSets[i])
			.setPImageInfo(&depthInfo)
			.setDescriptorType(vk::DescriptorType::eInputAttachment);
		std::array<vk::WriteDescriptorSet, 2> sets = { colorInput, depthInput };
		m_Device.GetLogicDevice().updateDescriptorSets(sets.size(), sets.data(), 0, nullptr);
	}
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