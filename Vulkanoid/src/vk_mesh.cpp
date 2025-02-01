#include "../headers/vk_mesh.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void VulkanMesh::SetStructures(VkDevice device,
	VkPhysicalDevice physDevice, VkCommandPool commandPool, VkQueue queue)
{
	this->_device = device;
	this->_physicalDevice = physDevice;
	this->_commandPool = commandPool;
	this->_queue = queue;

	// proceed model loading
	auto paths = FillVectorOfPaths();
	assert(paths.size() > 0, "Vector of model paths is empty!\n");
	
	for (uint32_t i = 0; i < paths.size(); ++i)
	{
		LoadModel(paths[i]);
	}

	// to do: more pipeline states and separate func
	if (_modelDesc.textures.size() >= 1)
	{
		_states.pushContstants = true;
	}

	_sampler = vkutil::CreateSampler(_device);
	_resources.PushFunction([=]
		{
			vkDestroySampler(_device, _sampler, nullptr);
		});
}


VulkanImage VulkanMesh::StbiLoadTexture(const char* fileName)
{
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VulkanImage image;

	if (pixels)
	{
		// would be better to convert to void to match memcpy format
		void* pixelsPtr = pixels;

		// format matches with stbi load parameter stbi_rgb_alpha
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		VulkanBuffer stagingBuffer;
		vkutil::CreateBuffer(_device, _physicalDevice, imageSize, stagingBuffer,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

		void* dataDst;
		vkMapMemory(_device, stagingBuffer.memory, 0, imageSize, 0, &dataDst);
		// driver may not copy data	 immediately this is why using host_coherent_bit is useful there
		memcpy(dataDst, pixelsPtr, (size_t)imageSize);
		vkUnmapMemory(_device, stagingBuffer.memory);

		stbi_image_free(pixels);


		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(texWidth);
		imageExtent.height = static_cast<uint32_t>(texHeight);
		imageExtent.depth = 1;

		VkImageCreateInfo imageInfo = vkinit::ImageCreateInfo(
			imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

		
		// sampled bit means we want to access image in the shader
		image = vkutil::CreateImage(_device, _physicalDevice, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkutil::CopyBufferToImage(_device, stagingBuffer.handle, image.handle, _commandPool, imageExtent, _queue);



		VkImageViewCreateInfo viewInfo = vkinit::ImageViewCreateInfo(imageFormat, image.handle,
			VK_IMAGE_ASPECT_COLOR_BIT);

		vkCreateImageView(_device, &viewInfo, nullptr, &image.view);



		vkDestroyBuffer(_device, stagingBuffer.handle,	nullptr);
		vkFreeMemory(_device, stagingBuffer.memory, nullptr);
		_resources.PushFunction([=]
			{
				vkDestroyImage(_device, image.handle, nullptr);
				vkDestroyImageView(_device, image.view, nullptr);
				vkFreeMemory(_device, image.memory, nullptr);
			});
	}

	if (image.handle != VK_NULL_HANDLE)
	{
		return image;
	}
	else return {};
}

void VulkanMesh::DrawMeshes(VkCommandBuffer cmd, VkPipelineLayout layout)
{
	VkDeviceSize offsets[1]{ 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &_geometryBuffers.vertices.handle, offsets);
	vkCmdBindIndexBuffer(cmd, _geometryBuffers.indices.handle, 0, VK_INDEX_TYPE_UINT32);
	// Draw indexed triangle

	for (uint32_t i = 0; i < _geometryBuffers.modelCount; ++i)
	{
		const uint32_t offset = _modelDesc.meshIndexOffset[i];
		if (_states.pushContstants)
		{
			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &_modelDesc.textureIds[i]);
		}

		vkCmdDrawIndexed(cmd, _modelDesc.currentMeshVertices[i], 1, offset, 0, 0);

	}
}


void VulkanMesh::CreateBuffers()
{
	VulkanBuffer scratchBuffer;
	vkutil::CreateBuffer(_device, _physicalDevice, 128 * 1024 * 1024, scratchBuffer,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // marking that buffer would be used as source of data
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	CreateVertexBuffer(scratchBuffer, _modelDesc.vertices);
	CreateIndexBuffer(scratchBuffer, _modelDesc.indices);


	vkDestroyBuffer(_device, scratchBuffer.handle, nullptr);
	vkFreeMemory(_device, scratchBuffer.memory, nullptr);

	_resources.PushFunction([=]
		{
			vkDestroyBuffer(_device, _geometryBuffers.vertices.handle, nullptr);
			vkFreeMemory(_device, _geometryBuffers.vertices.memory, nullptr);
			vkDestroyBuffer(_device, _geometryBuffers.indices.handle, nullptr);
			vkFreeMemory(_device, _geometryBuffers.indices.memory, nullptr);
		});
}



void VulkanMesh::CreateVertexBuffer(VulkanBuffer& scratchBuffer, std::vector<Vertex>& vertices)
{
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(vertices.size()) * sizeof(vertices[0]);

	assert(bufferSize > 0);
	assert(scratchBuffer.handle != VK_NULL_HANDLE);

	void* dataDst;
	vkMapMemory(_device, scratchBuffer.memory, 0, bufferSize, 0, &dataDst);
	// driver may not copy data	 immediately this is why using host_coherent_bit is useful there
	memcpy(dataDst, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, scratchBuffer.memory);


	vkutil::CreateBuffer(_device, _physicalDevice, bufferSize, _geometryBuffers.vertices,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // and this buffer is destination of data now
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // and this flag means that we are using the most appropriate memory type for GPU

	vkutil::CopyBuffer(_device, scratchBuffer.handle, _geometryBuffers.vertices.handle,
		bufferSize, _commandPool, _queue);

}

void VulkanMesh::CreateIndexBuffer(VulkanBuffer& scratchBuffer, std::vector<uint32_t>& indices)
{
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(indices.size()) * sizeof(indices[0]);

	assert(bufferSize > 0);
	assert(scratchBuffer.handle != VK_NULL_HANDLE);

	void* dataDst;
	vkMapMemory(_device, scratchBuffer.memory, 0, bufferSize, 0, &dataDst);
	memcpy(dataDst, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, scratchBuffer.memory);

	vkutil::CreateBuffer(_device, _physicalDevice, bufferSize,
		_geometryBuffers.indices, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkutil::CopyBuffer(_device, scratchBuffer.handle, _geometryBuffers.indices.handle, bufferSize,
		_commandPool, _queue);
}

void VulkanMesh::Cleanup()
{
	_resources.FlushQueue();
}


void VulkanMesh::LoadModel(std::filesystem::path& pathToModel)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(pathToModel.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "Assimp: unable to load your model\n" << importer.GetErrorString() << "\n";
		return;
	}


	ProcessNode(scene->mRootNode, scene);

}

void VulkanMesh::ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
{
	_modelDesc.vertices.reserve(aiMesh->mNumVertices);

	std::vector<Vertex> sortedVertices;
	sortedVertices.reserve(aiMesh->mNumVertices);
	for (uint32_t i = 0; i < aiMesh->mNumVertices; ++i)
	{
		Vertex vertices;
		vertices.position.x = aiMesh->mVertices[i].x;
		vertices.position.y = aiMesh->mVertices[i].y;
		vertices.position.z = aiMesh->mVertices[i].z;

		if (aiMesh->mTextureCoords[0])
		{
			vertices.uv.x = aiMesh->mTextureCoords[0][i].x;
			vertices.uv.y = aiMesh->mTextureCoords[0][i].y;
		}
		else { vertices.uv = glm::vec2(0.0f); }

		if (aiMesh->HasNormals())
		{
			vertices.normal.x = aiMesh->mNormals[i].x;
			vertices.normal.y = aiMesh->mNormals[i].y;
			vertices.normal.z = aiMesh->mNormals[i].z;
		}
		sortedVertices.push_back(vertices);
	}



	static uint32_t index = 0;
	_modelDesc.meshIndexOffset.push_back(index);

	uint32_t currentMeshSize = 0;
	for (uint32_t i = 0; i < aiMesh->mNumFaces; ++i)
	{
		aiFace face = aiMesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; ++j)
		{
			_modelDesc.indices.push_back(index);
			_modelDesc.vertices.push_back(sortedVertices[face.mIndices[j]]);


			++index;

			++currentMeshSize;
		}
	}

	_modelDesc.currentMeshVertices.push_back(currentMeshSize);

	if (aiMesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
		std::array<aiTextureType, 4> textureTypes
		{
			aiTextureType_DIFFUSE,
			aiTextureType_SPECULAR,
			aiTextureType_EMISSIVE,
			aiTextureType_HEIGHT
		};

		ProcessMaterial(material, textureTypes);
	}

}

void VulkanMesh::ProcessMaterial(aiMaterial* material, std::array<aiTextureType, 4> textureTypes)
{
	Texture textures{};
	// need it when loading a lot of models, for example 10 models
	// id should be unique for every texture
	static uint32_t offset = 1;

	const std::filesystem::path textureFolder = "objects/models/";
	std::pair<uint32_t, VulkanImage> textureDesc;

	PushConstant textureIds;
	for (uint32_t i = 0; i < textureTypes.size(); ++i)
	{
		const uint32_t currentTextureCount = material->GetTextureCount(textureTypes[i]);
		for (uint32_t j = 0; j < currentTextureCount; ++j)
		{
			aiString str;
			// to replace index later
			material->GetTexture(textureTypes[i], j, &str);
			textures.id = j + offset;
			textures.path = std::filesystem::absolute(textureFolder / str.C_Str());

			std::cout << "Texture path: " << textures.path << "\n";

			const uint32_t id = textures.id;
			textureDesc = std::make_pair(id, StbiLoadTexture(textures.path.string().c_str()));

			_modelDesc.textures.insert(textureDesc);

			switch (textureTypes[i])
			{
			case aiTextureType_DIFFUSE: textureIds.diffuseId = textures.id; break;
			case aiTextureType_SPECULAR: textureIds.specularId = textures.id; break;
			case aiTextureType_EMISSIVE: textureIds.emissiveId = textures.id; break;
			case aiTextureType_HEIGHT: textureIds.normalId = textures.id; break;
			}

		}
		offset += static_cast<uint32_t>(currentTextureCount);
	}

	_modelDesc.textureIds.push_back(textureIds);
	
}

void VulkanMesh::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene);
		++_geometryBuffers.modelCount;
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene);
	}
} 

std::vector<std::filesystem::path> VulkanMesh::FillVectorOfPaths()
{
	std::vector<std::filesystem::path> pathsToModels;
	std::filesystem::path model1 = "objects/models/character.obj";
	pathsToModels.push_back(model1);
	/*std::filesystem::path model2 = "objects/models/stoneGround.obj";
	pathsToModels.push_back(model2);*/
	std::filesystem::path model3 = "objects/models/ground.obj";
	pathsToModels.push_back(model3);



	return pathsToModels;
}

const VulkanoidOperations VulkanMesh::GetOperations() const
{
	VulkanoidOperations result = VulkanoidOperations::VULKANOID_NONE;

	if (_states.pushContstants)
	{
		result |= VulkanoidOperations::VULKANOID_USE_CONSTANTS;
	}

	return result;
}