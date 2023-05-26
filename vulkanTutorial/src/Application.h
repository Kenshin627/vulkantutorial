#pragma once

#include <vector>
#include <optional>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtx/hash.hpp>
#include "Window.h"
#include "vulkan/Device.h"
#include "vulkan/SwapChain.h"
#include "vulkan/Buffer.h"
#include "vulkan/FrameBuffer.h"
#include "vulkan/Shader.h"
#include "vulkan/Image.h"
#include "vulkan/Texture.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Color;
	glm::vec2 Coord;
	static vk::VertexInputBindingDescription GetBindingDescription()
	{
		vk::VertexInputBindingDescription desc;
		desc.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);
		return desc;
	}

	static std::vector<vk::VertexInputAttributeDescription> GetAttributes()
	{
		std::vector<vk::VertexInputAttributeDescription> result;
		vk::VertexInputAttributeDescription positionAttribute;
		positionAttribute.setBinding(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setLocation(0)
			.setOffset(offsetof(Vertex, Position));
		result.push_back(positionAttribute);

		vk::VertexInputAttributeDescription colorAttribute;
		colorAttribute.setBinding(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setLocation(1)
			.setOffset(offsetof(Vertex, Color));
		result.push_back(colorAttribute);

		vk::VertexInputAttributeDescription coordAttribute;
		coordAttribute.setBinding(0)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setLocation(2)
			.setOffset(offsetof(Vertex, Coord));
		result.push_back(coordAttribute);
		return result;
	}

	bool operator==(const Vertex& other) const
	{
		return Position == other.Position && Color == other.Color && Coord == other.Coord;
	}
};

struct UniformBufferObject
{
	glm::mat4 Proj;
	glm::mat4 View;
	glm::mat4 Model;
};

class Application
{
public:
	Application(int width, int height, const char* title) : m_Window(width, height, title)  {}

	void Run();
	void InitWindow(int width, int height, const char* title);
	void InitVulkan();
	void RenderLoop();
	void Clear();
	void LoadModel(const char* path);
private:
	void CreateInstance();
	void InitDevice(Window& window);
	void CreateSetLayout();
	void CreatePipeLine();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffer();
	void RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
	void CreateAsyncObjects();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void UpdateDescriptorSet();
	void DrawFrame();
	void UpdateUniformBuffers();
private:
	Window m_Window;
	vk::Instance m_VKInstance;
	Device m_Device;
	SwapChain m_SwapChain;

	vk::PipelineLayout m_Layout;
	vk::Pipeline m_Pipeline;
	
	vk::CommandBuffer m_CommandBuffer;
	vk::Fence m_InFlightFence;
	vk::Semaphore m_WaitAcquireImageSemaphore;
	vk::Semaphore m_WaitFinishDrawSemaphore;
	std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	std::vector<Vertex> m_VertexData /*= {
		{glm::vec3(-0.5, -0.5, 0.0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0, 1)},
		{glm::vec3( 0.5, -0.5, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1, 1)},
		{glm::vec3( 0.5,  0.5, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1, 0)},
		{glm::vec3(-0.5,  0.5, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0, 0)},

		{glm::vec3(-0.5, -0.5, 0.5), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0, 1)},
		{glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1, 1)},
		{glm::vec3(0.5,  0.5, 0.5), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1, 0)},
		{glm::vec3(-0.5,  0.5, 0.5), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0, 0)},
	}*/;
	std::vector<uint32_t> m_Indices /*= {
		0, 1, 2,
		2, 3, 0,
		4, 5, 6,
		6, 7, 4
	}*/;
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	Buffer m_UniformBuffer;
	
	vk::DescriptorSetLayout m_SetLayout;
	vk::DescriptorPool m_DescriptorPool;
	vk::DescriptorSet m_DescriptorSet;

	Texture m_Texture;

	vk::SampleCountFlagBits m_SamplerCount = vk::SampleCountFlagBits::e1;
};

namespace std
{
	template<>
	struct hash<Vertex>
	{
		size_t operator()(const Vertex& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.Position) ^
				(hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.Coord) << 1);
		}
	};
}