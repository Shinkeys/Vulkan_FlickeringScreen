// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "types/vk_types.h"

namespace vkinit
{
	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(
		VkCommandPool pool, uint32_t count = 1);

	VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	VkCommandBufferBeginInfo CommandBufferCreateInfo(VkCommandBufferUsageFlags flags = 0);

	VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

	VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
	VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
	VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
		VkSemaphoreSubmitInfo* waitSemaphoreInfo);

	// image creation
	VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

	// for imgui
	VkRenderingAttachmentInfo AttachmentInfo(
		VkImageView view, VkClearValue* clear, VkImageLayout layout);
	VkRenderingInfo RenderingInfo(
		VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);
	VkImageMemoryBarrier ImageMemoryBarrier();
}
