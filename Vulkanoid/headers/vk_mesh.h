#pragma once

#include "vk_types.h"
#include "vk_pipelines.h"
#include "vk_descriptor.h"
#include <vulkan/vulkan.h>

#include "../headers/types/VertexTypes.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>


constexpr uint32_t g_MAX_DRAW_COUNT = 1024;

struct Geometry
{
	VulkanBuffer vertices;
	VulkanBuffer indices;
	VulkanBuffer indirectCount;
	uint32_t modelCount{ 0 };
};

struct ModelDescriptor
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> meshIndexOffset;
	std::vector<uint32_t> currentMeshVertices;
};



class VulkanMesh
{
private:
	VkDevice _device;
	VkPhysicalDevice _physicalDevice;
	VkCommandPool _commandPool;
	VkQueue _queue;

	Geometry _geometryBuffers;

	ModelDescriptor _modelDesc;
	std::vector<VulkanImage> _textureDesc;


	VulkanImage StbiLoadTexture(const char* fileName);

	void CreateIndexBuffer(VulkanBuffer& scratchBuffer, std::vector<uint32_t>& indices);
	void CreateVertexBuffer(VulkanBuffer& scratchBuffer, std::vector<Vertex>& vertices);
	void CreateCountBuffer(VulkanBuffer& scratchBuffer, std::vector<uint32_t>& meshIndexOffset);

	VkSampler _sampler;


	// loading model itself
	void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	void LoadModel(std::filesystem::path& pathToModel);
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMaterial(aiMaterial* material, std::array<aiTextureType, 4> textureTypes);
	void SetupModelLoader(std::vector<std::filesystem::path>& pathsToModels);
	std::vector<std::filesystem::path> FillVectorOfPaths();



public:
	VulkanMesh()
	{
		auto paths = FillVectorOfPaths();
		assert(paths.size() > 0, "Vector of model paths is empty!");
		SetupModelLoader(paths);
	}
	void Cleanup();
	

	const std::vector<VulkanImage>& GetTextures() const { return _textureDesc; }
	const ModelDescriptor& GetModels() const { return _modelDesc; }

	const VkSampler GetSampler() const { return _sampler; }
	void DrawMeshes(VkCommandBuffer cmd);
	void CreateBuffers();
	void SetStructures(VkDevice device, VkPhysicalDevice physDevice, 
		VkCommandPool commandPool, VkQueue queue);

};