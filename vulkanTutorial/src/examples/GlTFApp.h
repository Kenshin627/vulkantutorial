#pragma once
#include "../vulkan/Device.h"
#include "../vulkan/SwapChain.h"
#include "../vulkan/Buffer.h"
#include "../vulkan/FrameBuffer.h"
#include "../vulkan/Shader.h"
#include "../vulkan/Image.h"
#include "../vulkan/Texture.h"
#include "../vulkan/CubeMap.h"
#include "../vulkan/PipelineLayout.h"
#include "../vulkan/RenderPass.h"
#include "../vulkan/glTFModel.h"
#include "../AppBase.h"

#include <vector>
#include <optional>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtx/hash.hpp>
#include "../core/EditorCamera.h"

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

struct CameraUniform
{
	glm::mat4 Proj;
	glm::mat4 View;
	glm::mat4 Model;
};

struct LightUniform
{
	alignas(16) glm::vec3 Direction;
	glm::vec3 Color;
};

struct PipeLines
{
	vk::Pipeline BlinnPhong;
	vk::Pipeline WireFrame;
};

class GLTFApp : public AppBase
{
public:
	GLTFApp(int width, int height, const char* title):AppBase(width, height, title) {}
	void Run();
	void InitContext();
	void RenderLoop();
	void Clear();
	void LoadModel(const char* path, std::vector<Vertex>& vertexData, std::vector<uint32_t>& indicesData);
	virtual void CreateSetLayout() override;
	virtual void RebuildFrameBuffer() override;
	std::vector<RenderPass>& GetRenderPass() { return m_RenderPasses; }
private:
	void CreatePipeLine();
	void CreateRenderPass();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffer();
	void RecordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
	void CreateAsyncObjects();

	void DrawFrame();
	void UpdateUniformBuffers();
private:
	Device m_Device;
	SwapChain m_SwapChain;
	vk::CommandBuffer m_CommandBuffer;
	vk::SampleCountFlagBits m_SamplerCount = vk::SampleCountFlagBits::e1;
	std::vector<RenderPass> m_RenderPasses;

	PipeLines m_PipeLines;
	RenderPass BlinnPhongPass;
	PipeLineLayout PipelineLayout;

	//signals
	vk::Fence m_InFlightFence;
	vk::Semaphore m_WaitAcquireImageSemaphore;
	vk::Semaphore m_WaitFinishDrawSemaphore;

	vk::DescriptorPool m_DescriptorPool;
	std::vector<vk::DescriptorPoolSize> m_PoolSizes;
	uint32_t m_SetCount = 0;
	
	//skyboxData
	std::vector<Vertex> m_SkyboxVexData;
	std::vector<uint32_t> m_SkyboxIndices;
	Buffer m_SkyboxVertexBuffer;
	Buffer m_SkyboxIndexBuffer;
	CubeMap m_SkyBoxTexture;

	Buffer m_CameraUniformBuffer;
	Buffer m_LightUniformBuffer;
	GlTFModel m_Model;
	EditorCamera m_Camera;
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