#include "../headers/vk_mesh.h"


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

		
	}

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
}