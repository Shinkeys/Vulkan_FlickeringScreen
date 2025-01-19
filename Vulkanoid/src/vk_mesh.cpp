#include "../headers/vk_mesh.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void VulkanMesh::SetVulkanStructures(VkDevice device, 
	VkPhysicalDevice physDevice, VkCommandPool commandPool, VkQueue queue)
{
	this->_device = device;
	this->_physicalDevice = physDevice;
	this->_commandPool = commandPool;
	this->_queue = queue;
}

void VulkanMesh::SetupMeshData()
{
	ModelLoader model;
	for (uint32_t i = 0; i < model.GetMeshData().size(); ++i)
	{
		_vertices.reserve(model.GetMeshData()[i].vertex.size());

		_vertices.insert(_vertices.end(), std::make_move_iterator(model.GetMeshData()[i].vertex.begin()),
			std::make_move_iterator(model.GetMeshData()[i].vertex.end()));


		_indices.reserve(model.GetMeshData()[i].indices.size());

		_indices.insert(_indices.end(), std::make_move_iterator(model.GetMeshData()[i].indices.begin()),
			std::make_move_iterator(model.GetMeshData()[i].indices.end()));

		
		Texture currentTexture = model.GetMeshData()[i].textures;
		_textures.push_back(StbiLoadTexture((currentTexture.diffuse_path.C_Str())));
		_textures.push_back(StbiLoadTexture((currentTexture.emissive_path.C_Str())));
		_textures.push_back(StbiLoadTexture((currentTexture.normal_path.C_Str())));
		_textures.push_back(StbiLoadTexture((currentTexture.specular_path.C_Str())));
	}

	_meshCount = model.GetMeshData().size();

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
		imageExtent.height = static_cast<uint32_t>(texWidth);
		imageExtent.depth = 1;

		VkImageCreateInfo imageInfo = vkinit::ImageCreateInfo(
			imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

		
		// sampled bit means we want to access image in the shader
		vkutil::CreateImage(_device, _physicalDevice, image, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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

void VulkanMesh::DrawMeshes(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
{
	for (uint32_t i = 0; i < _meshCount; ++i)
	{

	}
}

void VulkanMesh::Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
{
	//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &_uniformBuffers[_frameNumber].descriptorSet, 0, nullptr);
	//// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	//vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	//// Bind triangle vertex buffer (contains position and colors)
	//VkDeviceSize offsets[1]{ 0 };
	//vkCmdBindVertexBuffers(cmd, 0, 1, &_meshBuffer.vertices.handle, offsets);
	//// Bind triangle index buffer
	//vkCmdBindIndexBuffer(cmd, _meshBuffer.indices.handle, 0, VK_INDEX_TYPE_UINT32);
	//// Draw indexed triangle
}

void VulkanMesh::SetupBuffers()
{
	CreateVertexBuffer();
	CreateIndexBuffer();
}



void VulkanMesh::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_vertices[0]) * _vertices.size();

	VulkanBuffer stagingBuffer;
	vkutil::CreateBuffer(_device, _physicalDevice, bufferSize, stagingBuffer,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // marking that buffer would be used as source of data
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* dataDst;
	vkMapMemory(_device, stagingBuffer.memory, 0, bufferSize, 0, &dataDst);
	// driver may not copy data	 immediately this is why using host_coherent_bit is useful there
	memcpy(dataDst, _vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, stagingBuffer.memory);


	vkutil::CreateBuffer(_device, _physicalDevice, bufferSize, _meshBuffer.vertices,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // and this buffer is destination of data now
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // and this flag means that we are using the most appropriate memory type for GPU
	// copy vertex data to buff
	vkutil::CopyBuffer(_device, stagingBuffer.handle, _meshBuffer.vertices.handle,
		bufferSize, _commandPool, _queue);

	vkDestroyBuffer(_device, stagingBuffer.handle, nullptr);
	vkFreeMemory(_device, stagingBuffer.memory, nullptr);
}

void VulkanMesh::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_indices[0]) * _indices.size();

	VulkanBuffer stagingBuffer;
	vkutil::CreateBuffer(_device, _physicalDevice, bufferSize, stagingBuffer, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* dataDst;
	vkMapMemory(_device, stagingBuffer.memory, 0, bufferSize, 0, &dataDst);
	memcpy(dataDst, _indices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, stagingBuffer.memory);

	vkutil::CreateBuffer(_device, _physicalDevice, bufferSize,
		_meshBuffer.indices, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkutil::CopyBuffer(_device, stagingBuffer.handle, _meshBuffer.indices.handle, bufferSize,
		_commandPool, _queue);

	vkDestroyBuffer(_device, stagingBuffer.handle, nullptr);
	vkFreeMemory(_device, stagingBuffer.memory, nullptr);
}

void VulkanMesh::Cleanup()
{
	vkDestroyBuffer(_device, _meshBuffer.vertices.handle, nullptr);
	vkFreeMemory(_device, _meshBuffer.vertices.memory, nullptr);
	vkDestroyBuffer(_device, _meshBuffer.indices.handle, nullptr);
	vkFreeMemory(_device, _meshBuffer.indices.memory, nullptr);

	for (uint32_t i = 0; i < _textures.size(); ++i)
	{
		vkDestroyImage(_device, _textures[i].handle, nullptr);
		vkFreeMemory(_device, _textures[i].memory, nullptr);
	}
}