#pragma once
#include "Window.h"
#include "vulkan/Device.h"
#include "vulkan/SwapChain.h"
#include "vulkan/Buffer.h"
#include "vulkan/FrameBuffer.h"
#include "vulkan/Shader.h"
#include "vulkan/Image.h"
#include "vulkan/Texture.h"
#include "vulkan/CubeMap.h"
#include "vulkan/SetLayout.h"

#include <vector>
#include <optional>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtx/hash.hpp>

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

struct PipeLines
{
	vk::Pipeline Phong;
	vk::Pipeline GrayScale;
	vk::Pipeline WireFrame;
	vk::Pipeline PushConstant;
	vk::Pipeline SkyBox;
};

struct PushConsntantCube
{
	glm::vec3 Pos;
};

class Application
{
public:
	Application(int width, int height, const char* title) : m_Window(width, height, title)  {}
	void Run();
	void InitWindow(int width, int height, const char* title);
	void InitContext();
	void RenderLoop();
	void Clear();
	void LoadModel(const char* path, std::vector<Vertex>& vertexData, std::vector<uint32_t>& indicesData);
	void CreateSetLayout();
	void BuildAndUpdateDescriptorSets();
private:
	void CreatePipeLine();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffer();
	void RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
	void CreateAsyncObjects();

	void DrawFrame();
	void UpdateUniformBuffers();
private:
	Window m_Window;
	Device m_Device;
	SwapChain m_SwapChain;
	
	vk::CommandBuffer m_CommandBuffer;
	vk::Fence m_InFlightFence;
	vk::Semaphore m_WaitAcquireImageSemaphore;
	vk::Semaphore m_WaitFinishDrawSemaphore;
	vk::SampleCountFlagBits m_SamplerCount = vk::SampleCountFlagBits::e1;	
	PipeLines m_PipeLines;

	vk::DescriptorPool m_DescriptorPool;
	std::vector<vk::DescriptorPoolSize> m_PoolSizes;
	BindingSetLayout m_PushConstantSetlayout;
	std::vector<BindingSetLayout> m_InputAttachmentSetlayouts;
	
	uint32_t m_SetCount = 0;

	//GeometryData
	std::vector<Vertex> m_VertexData;
	std::vector<uint32_t> m_Indices;
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	Buffer m_UniformBuffer;
	Texture m_Texture;	

	std::vector<Vertex> m_CubeVexData;
	std::vector<uint32_t> m_CubeIndices;
	Buffer m_CubeVertexBuffer;	
	Buffer m_CubeIndexBuffer;
	Texture m_CubeTexture;

	CubeMap m_SkyBoxTexture;

	std::vector<PushConsntantCube> CubePushConstants =
	{
		{{ 0.0, 0.0, 0.0 }}
	};
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