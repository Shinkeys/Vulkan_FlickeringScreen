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

	_sampler = vkutil::CreateSampler(_device);
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
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

		
	}

	if (image.handle != VK_NULL_HANDLE)
	{
		return image;
	}
	else return {};
}

void VulkanMesh::DrawMeshes(VkCommandBuffer cmd)
{
	VkDeviceSize offsets[1]{ 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &_geometryBuffers.vertices.handle, offsets);
	vkCmdBindIndexBuffer(cmd, _geometryBuffers.indices.handle, 0, VK_INDEX_TYPE_UINT32);
	// Draw indexed triangle
	for (uint32_t i = 0; i < _geometryBuffers.modelCount; ++i)
	{
		const uint32_t offset = _modelDesc.meshIndexOffset[i];
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
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT); // and this flag means that we are using the most appropriate memory type for GPU

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
	vkDestroyBuffer(_device, _geometryBuffers.vertices.handle, nullptr);
	vkFreeMemory(_device, _geometryBuffers.vertices.memory, nullptr);
	vkDestroyBuffer(_device, _geometryBuffers.indices.handle, nullptr);
	vkFreeMemory(_device, _geometryBuffers.indices.memory, nullptr);

	//for (uint32_t i = 0; i < _textures.size(); ++i)
	//{
	//	vkDestroyImage(_device, _textures[i].handle, nullptr);
	//	vkFreeMemory(_device, _textures[i].memory, nullptr);
	//}

	vkDestroySampler(_device, _sampler, nullptr);
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
	static uint32_t offset = 0;
	for (uint32_t i = 0; i < textureTypes.size(); ++i)
	{
		const uint32_t currentTextureCount = material->GetTextureCount(textureTypes[i]);
		for (uint32_t j = 0; j < currentTextureCount; ++j)
		{
			aiString str;
			// to replace index later
			material->GetTexture(textureTypes[i], j, &str);
			textures.id = j + offset;
			textures.path = str;

		}
		offset += static_cast<uint32_t>(currentTextureCount);
	}

	// loading textures and storing its data
	// to do: put it to ModelDescriptor struct
	/*_textureDesc.push_back(StbiLoadTexture(textures.path.C_Str()));*/
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

void VulkanMesh::SetupModelLoader(std::vector<std::filesystem::path>& pathsToModels)
{
	for (uint32_t i = 0; i < pathsToModels.size(); ++i)
	{
		LoadModel(pathsToModels[i]);
	}
}
std::vector<std::filesystem::path> VulkanMesh::FillVectorOfPaths()
{
	std::vector<std::filesystem::path> pathsToModels;
	/*std::filesystem::path model1 = "objects/models/blacksmith/scene.gltf";
	_pathToModels.push_back(model1);*/
	std::filesystem::path model2 = "objects/models/character.obj";
	pathsToModels.push_back(model2);


	return pathsToModels;
}