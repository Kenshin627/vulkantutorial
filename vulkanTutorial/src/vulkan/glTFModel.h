#pragma once

#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include "Device.h"
#include "Texture.h"
#include "Buffer.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class GlTFModel
{
public:
	GlTFModel() = default;

	void LoadModel(Device& device, const std::string& filaname);
	void Draw(vk::CommandBuffer command, vk::PipelineLayout layout);
	~GlTFModel()
	{
		m_VertexBuffer.Clear();
		m_IndexBuffer.Clear();
		for (auto& image : m_Textures)
		{
			image.Clear();
		}
	}
public:
	struct Vertex
	{
		glm::vec3 Pos;
		glm::vec3 Normal;
		glm::vec2 Coords;
		glm::vec3 Color;
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
	
	std::vector<Texture> m_Textures;
	std::vector<GlTFModel::TextureIndex> m_TextureIndices;
	std::vector<Material> m_Materials;
	std::vector<Node*> m_Nodes;
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	std::vector<uint32_t> m_Indices;
	std::vector<GlTFModel::Vertex> m_Vertices;
};