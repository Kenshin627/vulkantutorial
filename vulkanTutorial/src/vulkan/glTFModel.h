#pragma once

#include "Device.h"
#include "Texture.h"
#include "Buffer.h"
#include "PipelineLayout.h"
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
		/*	glm::vec3 Color;
			glm::vec4 Tangent;*/
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
			//vk::VertexInputAttributeDescription colorDesc = { 3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Color) };
			//vk::VertexInputAttributeDescription tangentDesc = { 4, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, Tangent) };
			std::vector< vk::VertexInputAttributeDescription> res;
			res.push_back(posDesc);
			res.push_back(normalDesc);
			res.push_back(coordDesc);
			//res.push_back(colorDesc);
			//res.push_back(tangentDesc);
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
		glm::vec4 BaseColorFactor;
		uint32_t  BaseColorTextureIndex;
		float MetallicFactor;
		float RoughnessFactor;
		uint32_t  MetallicRoughnessTextureIndex;
		float OcclustionStrength;
		uint32_t OcclusionTextureIndex;
		float NormalScale;
		uint32_t  NormalMapTextureIndex;
		glm::vec3 EmissiveFactor;
		uint32_t EmissiveTextureIndex;
	};

	struct ModelMatrix
	{
		glm::mat4 matrix;
	};

	struct PBRFactor
	{
		glm::vec4 BaseColorFactor;
		float MetallicFactor;
		float RoughnessFactor;
		float OcclustionStrength;
		float NormalScale;
		glm::vec3 EmissiveFactor;
	};

	struct TextureIndex
	{
		int32_t ImageIndex;
	};
public:
	GlTFModel() = default;

	void LoadModel(Device& device, const std::string& filaname);
	void Draw(vk::CommandBuffer command, PipeLineLayout& layout);
	uint32_t GetTextureCount() { return m_Textures.size(); }
	std::vector<Texture>& GetImages() { return m_Textures; }
	DescriptorSetLayoutCreateInfo GetDescriptorSet() { return m_DescriptorSetLayout; }
	~GlTFModel()
	{
		m_VertexBuffer.Clear();
		m_IndexBuffer.Clear();
		for (auto& image : m_Textures)
		{
			image.Clear();
		}
	}
private:
	void LoadImages();
	void LoadMaterials();
	void loadTextures();
	void LoadNode(const tinygltf::Node& inputNode, GlTFModel::Node* parent);
	void DrawNode(Node* node, vk::CommandBuffer command, PipeLineLayout& layout);
	void UpdateUniforms(uint32_t matId, glm::mat4 modelMatrix);
	void BuildDescriptorSets();
	
private:
	Device m_Device;
	tinygltf::Model m_Model;
	tinygltf::TinyGLTF m_Contenxt;
	std::string err;
	std::string warning;
	DescriptorSetLayoutCreateInfo m_DescriptorSetLayout;
	std::vector<Texture> m_Textures;
	std::vector<GlTFModel::TextureIndex> m_TextureIndices;
	std::vector<Material> m_Materials;
	std::vector<PBRFactor> m_PBRFactors;
	std::vector<Node*> m_Nodes;
	Buffer m_VertexBuffer;
	Buffer m_IndexBuffer;
	Buffer m_UniformBuffer;
	std::vector<Buffer> m_ModelMatrixs;
	std::vector<uint32_t> m_Indices;
	std::vector<GlTFModel::Vertex> m_Vertices;
};