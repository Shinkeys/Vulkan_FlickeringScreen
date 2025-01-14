#pragma once

#include <vulkan/vulkan.h>

namespace vkutil
{
	void transition_image(VkCommandBuffer cmd, VkImage image,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
		VkImageLayout currentLayout, VkImageLayout nextLayout,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange);

	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination,
		VkExtent2D srcSize, VkExtent2D destSize);
}
