// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include "../vendor/vma/vk_mem_alloc.h"
#include "../vendor/glm/glm/glm.hpp"

#include <iostream>

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
	VmaAllocation allocation;
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


static int _frameNumber{ 0 };
constexpr unsigned int MAX_CONCURRENT_FRAMES = 2;

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

