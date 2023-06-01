#pragma once

#include "Device.h"
#include "Texture.h"
#include "Buffer.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class GlTFModel
{
public:
	struct Vertex
	{
		glm::vec3 Pos;
		glm::vec3 Normal;
		glm::vec2 Coords;
		glm::vec3 Color;
		static std::vector<vk::VertexInputBindingDescription> GetBindingDescriptions()
		{
			vk::VertexInputBindingDescription vertexInputBindings;
			vertexInputBindings.setBinding(0)
				.setInputRate(vk::VertexInputRate::eVertex)
				.setStride(sizeof(Vertex));
			std::vector<vk::VertexInputBindingDescription> res;
			res.push_back(vertexInputBindings);
			return res;
		}

		static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions()
		{
			vk::VertexInputAttributeDescription posDesc = { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Pos) };
			vk::VertexInputAttributeDescription normalDesc = { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Normal) };
			vk::VertexInputAttributeDescription coordDesc = { 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, Coords) };
			vk::VertexInputAttributeDescription colorDesc = { 3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Color) };
			std::vector< vk::VertexInputAttributeDescription> res;
			res.push_back(posDesc);
			res.push_back(normalDesc);
			res.push_back(coordDesc);
			res.push_back(colorDesc);
			return res;
		}
	};

	struct Primitive
	{
		uint32_t FirstIndex;
		uint32_t IndexCount;
		uint32_t MaterialIndex;
	};

	struct Mesh
	{
		std::vector<Primitive> Primitives;
	};

	struct Node
	{
		Node* Parent;
		std::vector<Node*> Children;
		Mesh NodeMesh;
		glm::mat4 ModelMatrix;
		~Node()
		{
			for (auto& child : Children)
			{
				delete child;
			}
		}
	};

	struct Material
	{
		glm::vec4 BaseColorFactor = glm::vec4(1.0f);
		uint32_t  BaseColorTextureIndex;
	};

	struct TextureIndex
	{
		int32_t ImageIndex;
	};

	struct ImageSet
	{
		Texture Texture;
		vk::DescriptorSet DescSet;
	};
public:
	GlTFModel() = default;

	void LoadModel(Device& device, const std::string& filaname);
	void Draw(vk::CommandBuffer command, vk::PipelineLayout layout);
	uint32_t GetTextureCount() { return m_Textures.size(); }
	std::vector<ImageSet>& GetImageSet() { return m_Textures; }
	~GlTFModel()
	{
		m_VertexBuffer.Clear();
		m_IndexBuffer.Clear();
		for (auto& image : m_Textures)
		{
			image.Texture.Clear();
		}
	}
private:
	void LoadImages();
	void LoadMaterials();
	void loadTextures();
	void LoadNode(const tinygltf::Node& inputNode, GlTFModel::Node* parent);
	void DrawNode(Node* node, vk::CommandBuffer command, vk::PipelineLayout layout);
private:
	Device m_Device;
	tinygltf::Model m_Model;
	tinygltf::TinyGLTF m_Contenxt;
	std::string err;
	std::string warning;
	
	std::vector<ImageSet> m_Textures;
	std::vector<GlTFModel::TextureIndex> m_TextureIndices;
	std::vector<Material> m_Materials;
	std::vector<Node*> m_Nodes;
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	std::vector<uint32_t> m_Indices;
	std::vector<GlTFModel::Vertex> m_Vertices;
};