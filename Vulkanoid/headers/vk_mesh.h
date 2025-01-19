#pragma once

#include "systems/model.h"
#include "vk_types.h"
#include "vk_pipelines.h"
#include "vk_descriptor.h"
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
	std::vector<uint32_t> _currentMeshSize;
	uint32_t _meshCount;
	

	void SetupMeshData();
	void Cleanup();

	VulkanImage StbiLoadTexture(const char* fileName);

	void CreateIndexBuffer();
	void CreateVertexBuffer();
	void Draw(VkCommandBuffer cmd, uint32_t indicesCount);
	
	VkSampler _sampler;
public:
	const std::vector<VulkanImage> GetTextures() const { return _textures; }
	const uint32_t GetMeshCount() const { return _meshCount; }
	const VkSampler GetSampler() const { return _sampler; }
	void DrawMeshes(VkCommandBuffer cmd);
	const MeshBuffer& GetBuffers() const { return _meshBuffer; }
	void CreateBuffers();
	void SetStructures(VkDevice device, VkPhysicalDevice physDevice, 
		VkCommandPool commandPool, VkQueue queue);

};