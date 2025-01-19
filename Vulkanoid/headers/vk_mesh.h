#pragma once

#include "systems/model.h"
#include "vk_types.h"
#include "vk_pipelines.h"
#include <vulkan/vulkan.h>


class VulkanMesh
{
private:
	VkDevice _device;
	VkPhysicalDevice _physicalDevice;
	VkCommandPool _commandPool;
	VkQueue _queue;

	MeshBuffer _meshBuffer;
	std::vector<VulkanImage> _textures;
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	uint32_t _meshCount;

	void SetupMeshData();
	void Cleanup();
	// one ubo per frame, so we can have overframe overlap to be sure uniforms arent updated while still in use
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> _uniformBuffers;
	VulkanImage StbiLoadTexture(const char* fileName);

	void CreateIndexBuffer();
	void CreateVertexBuffer();
	void Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
public:
	void DrawMeshes(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
	const MeshBuffer& GetBuffers() const { return _meshBuffer; }
	const uint32_t& GetIndicesCount() const{ return static_cast<uint32_t>(_indices.size()); }
	const uint32_t& GetVerticesCount() const{ return static_cast<uint32_t>(_vertices.size()); }
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