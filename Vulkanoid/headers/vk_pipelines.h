#pragma once
#include <fstream>
#include "vk_initializers.h"
#include "vk_tools.h"
#include "types/vk_types.h"

#include <vector>
#include <array>

namespace vkutil
{
	VkShaderModule LoadShader(const char* filePath, VkDevice device);

	void CreateBuffer(VkDevice device, VkPhysicalDevice physDevice, 
		VkDeviceSize deviceSize, VulkanBuffer& bufferData,
		VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);

	VulkanImage CreateImage(VkDevice device, VkPhysicalDevice physDevice, VkImageCreateInfo imageInfo,
		VkMemoryPropertyFlags memFlags);

	void CopyBufferToImage(VkDevice device, VkBuffer srcBuffer, VkImage dstImage,
		VkCommandPool cmdPool, VkExtent3D extent, VkQueue queue);


	void CopyBuffer(VkDevice device, VkBuffer srcBuffer,
		VkBuffer dstBuffer, VkDeviceSize size,
		VkCommandPool cmdPool, VkQueue queue);

	VkSampler CreateSampler(VkDevice device);

	void SubmitToQueue(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkCommandBuffer commandBuffer);

	// image
	void TransitionImage(VkCommandBuffer cmd, VkImage image,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
		VkImageLayout currentLayout, VkImageLayout nextLayout,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange);

	void CopyImgToImg(VkCommandBuffer cmd, VkImage source, VkImage destination,
		VkExtent2D srcSize, VkExtent2D destSize);

	std::array<UniformBuffer, g_MAX_CONCURRENT_FRAMES> CreateUniformBuffers(VkDevice device, VkPhysicalDevice physDevice);
}