// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include "../vendor/vma/vk_mem_alloc.h"

#include <iostream>

//we will add our main reusable types here

struct VulkanBuffer
{
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkBuffer handle{ VK_NULL_HANDLE };
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

