#include "../Core.h"
#include "glTFModel.h"
#include <gtc/type_ptr.hpp>

void GlTFModel::LoadModel(Device& device, const std::string& filaname)
{
	bool isLoaded = m_Contenxt.LoadASCIIFromFile(&m_Model, &err, &warning, filaname);
	if (!isLoaded)
	{
		throw std::runtime_error("error load gtTF!");
	}
	m_Device = device;
	LoadImages();
	LoadMaterials();
	loadTextures();
	tinygltf::Scene& scene = m_Model.scenes[0];
	for (uint32_t i = 0; i < scene.nodes.size(); i++)
	{
		tinygltf::Node& node = m_Model.nodes[scene.nodes[i]];
		LoadNode(node, nullptr);
	}

	//buffers
	vk::DeviceSize vertexBufferSize = sizeof(GlTFModel::Vertex) * m_Vertices.size();
	Buffer vertexStagingBuffer;
	vertexStagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, vertexBufferSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Vertices.data());
	m_VertexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vertexBufferSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	Buffer::CopyBuffer(vertexStagingBuffer.m_Buffer, 0, m_VertexBuffer.m_Buffer, 0, vertexBufferSize, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());


	vk::DeviceSize indexBufferSize = sizeof(uint32_t) * m_Indices.size();
	Buffer indexStagingBuffer;
	indexStagingBuffer.Create(m_Device, vk::BufferUsageFlagBits::eTransferSrc, indexBufferSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Indices.data());
	m_IndexBuffer.Create(m_Device, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, indexBufferSize, vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eDeviceLocal, nullptr);
	Buffer::CopyBuffer(indexStagingBuffer.m_Buffer, 0, m_IndexBuffer.m_Buffer, 0, indexBufferSize, m_Device.GetGraphicQueue(), m_Device.GetCommandManager());

	m_UniformBuffer.Create(m_Device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(PBRFactor), vk::SharingMode::eExclusive, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, nullptr);
	m_UniformBuffer.Map();
	BuildDescriptorSets();
}

void GlTFModel::LoadImages()
{
	uint32_t imageCount = m_Model.images.size();
	m_Textures.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		tinygltf::Image& gltfImage = m_Model.images[i];
		unsigned char* bufferData = nullptr;
		vk::DeviceSize bufferSize = 0;
		int pixelCount = 0;
		bool deleteBuffer = false;
		if (gltfImage.component == 3)
		{
			deleteBuffer = true;
			pixelCount = gltfImage.width * gltfImage.height;
			bufferSize = pixelCount * 4;
			bufferData = new unsigned char[bufferSize];
			unsigned char* rgba = bufferData;
			unsigned char* rgb = &gltfImage.image[0];
			for (uint32_t j = 0; j < pixelCount; j++)
			{
				memcpy(rgba, rgb, sizeof(unsigned char) * 3);
				rgba += 4;
				rgb  += 3;
			}
		}
		else
		{
			bufferData = &gltfImage.image[0];
			bufferSize = gltfImage.image.size() * sizeof(unsigned char);
		}
		m_Textures[i].FromBuffer(m_Device, bufferData, bufferSize, vk::Format::eR8G8B8A8Srgb, gltfImage.width, gltfImage.height, false);
		if (deleteBuffer)
		{
			delete[] bufferData;
		}
	}
}

void GlTFModel::LoadMaterials()
{
	uint32_t materialCount = m_Model.materials.size();
	m_Materials.resize(materialCount);
	m_PBRFactors.resize(materialCount);
	for (uint32_t i = 0; i < materialCount; i++)
	{
		tinygltf::Material& gltfMat = m_Model.materials[i];
		glm::vec4 baseColor = glm::make_vec4(gltfMat.pbrMetallicRoughness.baseColorFactor.data());
		m_Materials[i].BaseColorFactor = baseColor;
		m_PBRFactors[i].BaseColorFactor = baseColor;
		if (gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			m_Materials[i].BaseColorTextureIndex = static_cast<uint32_t>(gltfMat.pbrMetallicRoughness.baseColorTexture.index);
		}
		m_Materials[i].MetallicFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.metallicFactor);
		m_PBRFactors[i].MetallicFactor = m_Materials[i].MetallicFactor;

		m_Materials[i].RoughnessFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.roughnessFactor);
		m_PBRFactors[i].RoughnessFactor = m_Materials[i].RoughnessFactor;
		if (gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		{
			m_Materials[i].MetallicRoughnessTextureIndex = static_cast<uint32_t>(gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index);
		}
		m_Materials[i].OcclustionStrength = static_cast<float>(gltfMat.occlusionTexture.strength);
		m_PBRFactors[i].OcclustionStrength = m_Materials[i].OcclustionStrength;

		if (gltfMat.occlusionTexture.index >= 0)
		{
			m_Materials[i].OcclusionTextureIndex = static_cast<uint32_t>(gltfMat.occlusionTexture.index);
		}
		if (gltfMat.normalTexture.index >= 0)
		{
			m_Materials[i].NormalScale = gltfMat.normalTexture.scale;
			m_Materials[i].NormalMapTextureIndex = static_cast<uint32_t>(gltfMat.normalTexture.index);
			m_PBRFactors[i].NormalScale = m_Materials[i].NormalScale;
		}
		if (gltfMat.emissiveTexture.index >= 0)
		{
			m_Materials[i].EmissiveFactor = glm::make_vec3(gltfMat.emissiveFactor.data());
			m_Materials[i].EmissiveTextureIndex = static_cast<uint32_t>(gltfMat.emissiveTexture.index);
			m_PBRFactors[i].EmissiveFactor = m_Materials[i].EmissiveFactor;
		}
	}
}

void GlTFModel::loadTextures()
{
	m_TextureIndices.resize(m_Model.textures.size());
	for (uint32_t i = 0; i < m_Model.textures.size(); i++)
	{
		m_TextureIndices[i].ImageIndex = m_Model.textures[i].source;
	}
}

void GlTFModel::LoadNode(const tinygltf::Node& inputNode, GlTFModel::Node* parent)
{
	GlTFModel::Node* node = new GlTFModel::Node();
	node->ModelMatrix = glm::mat4(1.0f);
	node->Parent = parent;
	if (inputNode.translation.size() == 3)
	{
		node->ModelMatrix = glm::translate(node->ModelMatrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}
	if (inputNode.rotation.size() == 4)
	{
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		node->ModelMatrix *= glm::mat4(q);
	}
	if (inputNode.scale.size() == 3)
	{
		node->ModelMatrix = glm::scale(node->ModelMatrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}
	if (inputNode.matrix.size() > 0)
	{
		node->ModelMatrix = glm::make_mat4(inputNode.matrix.data());
	}
	if (inputNode.children.size() > 0)
	{
		for (uint32_t i = 0; i < inputNode.children.size(); i++)
		{
			LoadNode(m_Model.nodes[inputNode.children[i]], node);
		}
	}
	if (inputNode.mesh > -1)
	{
		const tinygltf::Mesh& mesh = m_Model.meshes[inputNode.mesh];
		for (uint32_t i = 0; i < mesh.primitives.size(); i++)
		{
			const tinygltf::Primitive& primitive = mesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(m_Indices.size());
			uint32_t vertexStart = static_cast<uint32_t>(m_Vertices.size());
			uint32_t indexCount = 0;

			//vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalBuffer   = nullptr;
				const float* texCoordBuffer = nullptr;
				const float* tangentBuffer  = nullptr;
				uint32_t vertexCount = 0;
				if (primitive.attributes.find("POSITION") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = m_Model.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = m_Model.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(m_Model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = m_Model.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView view = m_Model.bufferViews[accessor.bufferView];
					normalBuffer = reinterpret_cast<const float*>(&(m_Model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = m_Model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = m_Model.bufferViews[accessor.bufferView];
					texCoordBuffer = reinterpret_cast<const float*>(&(m_Model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = m_Model.accessors[primitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& view = m_Model.bufferViews[accessor.bufferView];
					tangentBuffer = reinterpret_cast<const float*>(&(m_Model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					Vertex vertex;
					vertex.Pos = glm::vec4(glm::make_vec3(&positionBuffer[i * 3]), 1.0f);
					vertex.Normal = glm::normalize(glm::vec3(normalBuffer ? glm::make_vec3(&normalBuffer[i * 3]) : glm::vec3(0.0f)));
					vertex.Coords = texCoordBuffer ? glm::make_vec2(&texCoordBuffer[i * 2]) : glm::vec2(0.0f, 0.0f);
		/*			vertex.Color = glm::vec3(1.0f);
					vertex.Tangent = tangentBuffer ? glm::make_vec4(&tangentBuffer[i * 4]) : glm::vec4(0.0);*/
					m_Vertices.push_back(vertex);
				}
			}

			//indices
			{
				const tinygltf::Accessor& accessor = m_Model.accessors[primitive.indices];
				const tinygltf::BufferView& view = m_Model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = m_Model.buffers[view.buffer];
				indexCount += static_cast<uint32_t>(accessor.count);
				switch (accessor.componentType)
				{
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
						for (uint32_t i = 0; i < accessor.count; i++)
						{
							m_Indices.push_back(buf[i] + vertexStart);
						}
						break;
					}
					
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
						for (uint32_t i = 0; i < accessor.count; i++)
						{
							m_Indices.push_back(buf[i] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
						for (uint32_t i = 0; i < accessor.count; i++)
						{
							m_Indices.push_back(buf[i] + vertexStart);
						}
						break;
					}
				}
			}
			Primitive curPrimitive;
			curPrimitive.FirstIndex = firstIndex;
			curPrimitive.IndexCount = indexCount;
			curPrimitive.MaterialIndex = primitive.material;
			node->NodeMesh.Primitives.push_back(curPrimitive);
		}
	}

	if (parent)
	{
		parent->Children.push_back(node);
	}
	else
	{
		m_Nodes.push_back(node);
	}
}

void GlTFModel::Draw(vk::CommandBuffer command, PipeLineLayout& layout)
{
	vk::DeviceSize offset = 0.0f;
	command.bindVertexBuffers(0, 1, &m_VertexBuffer.m_Buffer, &offset);
	command.bindIndexBuffer(m_IndexBuffer.m_Buffer, offset, vk::IndexType::eUint32);
	for (auto& node : m_Nodes)
	{
		DrawNode(node, command, layout);
	}
}

void GlTFModel::DrawNode(Node* node, vk::CommandBuffer command, PipeLineLayout& layout)
{
	glm::mat4 modelMatrix = node->ModelMatrix;
	Node* parent = node->Parent;
	while (parent)
	{
		modelMatrix = parent->ModelMatrix * modelMatrix;
		parent = parent->Parent;
	}
	if (node->NodeMesh.Primitives.size() > 0)
	{
		for (uint32_t i = 0; i < node->NodeMesh.Primitives.size(); i++) 
		{
			auto& primitive = node->NodeMesh.Primitives[i];
			if (primitive.IndexCount > 0)
			{
				//bindSets
				uint32_t materialIndex = primitive.MaterialIndex;
				auto set = layout.GetDescriptorSet(1, materialIndex);
				command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout.GetPipelineLayout(), 0, 1, &set, 0, nullptr);
				UpdateUniforms(materialIndex);
				command.drawIndexed(primitive.IndexCount, 1, primitive.FirstIndex, 0, 0);
			}
		}
	}
	for (auto& child : node->Children)
	{
		DrawNode(child, command, layout);
	}
}

void GlTFModel::UpdateUniforms(uint32_t matId)
{
	m_UniformBuffer.CopyFrom(&m_PBRFactors[matId], sizeof(PBRFactor));
}

void GlTFModel::BuildDescriptorSets()
{
	m_DescriptorSetLayout.Bindings = {
		{ vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment, 0 }, //pbrFactor
		{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 1 }, //BaseColorTextureIndex
		{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 2 }, //MetallicRoughnessTextureIndex
		{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 3 }, //OcclusionTextureIndex
		//{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 4 }, //NormalMapTextureIndex
		//{ vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 5 }, //EmissiveTextureIndex
	};
	uint32_t setCount = m_Materials.size();
	m_DescriptorSetLayout.SetCount = setCount;
	for (uint32_t i = 0; i < setCount; i++)
	{
		m_DescriptorSetLayout.SetWriteData.push_back({
			{ m_UniformBuffer.m_Descriptor, {}, false },
			{ {}, m_Textures[m_TextureIndices[m_Materials[i].BaseColorTextureIndex].ImageIndex].GetDescriptor(), true },
			{ {}, m_Textures[m_TextureIndices[m_Materials[i].MetallicRoughnessTextureIndex].ImageIndex].GetDescriptor(), true },
			{ {}, m_Textures[m_TextureIndices[m_Materials[i].OcclusionTextureIndex].ImageIndex].GetDescriptor(), true },
		});
	}
}