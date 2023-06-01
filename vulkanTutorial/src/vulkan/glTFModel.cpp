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
			delete bufferData;
		}
	}
}

void GlTFModel::LoadMaterials()
{
	uint32_t materialCount = m_Model.materials.size();
	m_Materials.resize(materialCount);
	for (uint32_t i = 0; i < materialCount; i++)
	{
		tinygltf::Material& gltfMat = m_Model.materials[i];
		if (gltfMat.values.find("baseColorFactor") != gltfMat.values.end())
		{
			m_Materials[i].BaseColorFactor = glm::make_vec4(gltfMat.values["baseColorFactor"].ColorFactor().data());
		}

		if (gltfMat.values.find("baseColorTexture") != gltfMat.values.end())
		{
			m_Materials[i].BaseColorTextureIndex = static_cast<uint32_t>(gltfMat.values["baseColorTexture"].TextureIndex());
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
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					Vertex vertex;
					vertex.Pos = glm::vec4(glm::make_vec3(&positionBuffer[i * 3]), 1.0f);
					vertex.Normal = glm::normalize(glm::vec3(normalBuffer ? glm::make_vec3(&normalBuffer[i * 3]) : glm::vec3(0.0f)));
					vertex.Coords = texCoordBuffer ? glm::make_vec2(&texCoordBuffer[i * 2]) : glm::vec2(0.0f, 0.0f);
					vertex.Color = glm::vec3(1.0f);
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
		m_Modes.push_back(node);
	}
}