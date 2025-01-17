#pragma once

#include "systems/model.h"
#include "vk_types.h"
#include "vk_pipelines.h"
#include <vulkan/vulkan.h>

struct MeshBuffer
{
	VulkanBuffer vertices;
	VulkanBuffer indices;
};


class VulkanMesh
{
private:
	VkDevice _device;
	VkPhysicalDevice _physicalDevice;
	VkCommandPool _commandPool;
	VkQueue _queue;

	MeshBuffer _meshBuffer;
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;

	void SetupMeshData();
	void Cleanup();


	void CreateIndexBuffer();
	void CreateVertexBuffer();
public:
	const MeshBuffer& GetBuffers() const { return _meshBuffer; }
	const uint32_t& GetIndicesCount() const{ return _indices.size(); }
	const uint32_t& GetVerticesCount() const{ return _vertices.size(); }
	void SetupBuffers();
	void SetVulkanStructures(VkDevice device, VkPhysicalDevice physDevice, VkCommandPool commandPool, VkQueue queue);
	VulkanMesh()
	{
		SetupMeshData();
	}
	~VulkanMesh()
	{
		Cleanup();
	}

};