// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include "../../vendor/glm/glm/glm.hpp"

#include <iostream>
#include <map>

//we will add our main reusable types here

struct VulkanBuffer
{
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkBuffer handle{ VK_NULL_HANDLE };
};

struct VulkanImage
{
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkImage handle{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
};

struct MeshBuffer
{
	VulkanBuffer vertices;
	VulkanBuffer indices;
};

struct AllocatedImage
{
	VkImage image;
	VkImageView imageView;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
};

struct DepthStencil
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

// uniform buffer block object
struct UniformBuffer : VulkanBuffer
{
	// descriptor set stores the resources bound to the binding points in shader
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

	uint8_t* mapped{ nullptr };
};


struct ShaderData
{
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
};


static int g_frameNumber{ 0 };
constexpr unsigned int g_MAX_CONCURRENT_FRAMES = 2;
constexpr uint32_t g_API_VERSION = VK_API_VERSION_1_3;

// macro to check for Vulkan iteractions errors
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err{x};                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace vkdebug
{
#ifdef NDEBUG
	constexpr bool validationLayers = false;
#else
	constexpr bool validationLayers = true;
#endif
}



enum class VulkanoidOperations
{
	VULKANOID_NONE = 0x00000000,
	VULKANOID_USE_CONSTANTS = 0x00000001,
	VULKANOID_RESIZE_SWAPCHAIN = 0x00000002
};

// TO DO: push array of textures with getting id in the shader
struct PushConstant
{
	uint32_t diffuseId{0};
	uint32_t specularId{0};
	uint32_t emissiveId{0};
	uint32_t normalId{0};
};

inline VulkanoidOperations& operator|=(VulkanoidOperations& operationF, VulkanoidOperations operationS)
{
	operationF = static_cast<VulkanoidOperations>(static_cast<int>(operationF) | static_cast<int>(operationS));
	return operationF;
}

inline bool operator&(VulkanoidOperations operationF, VulkanoidOperations operationS)
{
	return static_cast<bool>(static_cast<int>(operationF) & static_cast<int>(operationS));
}