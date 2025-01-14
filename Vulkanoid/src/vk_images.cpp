#include "../headers/vk_images.h"	
#include "../headers/vk_initializers.h"

void vkutil::transition_image(VkCommandBuffer cmd, VkImage image, 
	VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
	VkImageLayout currentLayout, VkImageLayout nextLayout,
	VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
	VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier imageMemoryBarrier = vkinit::imageMemoryBarrier();
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = currentLayout;
	imageMemoryBarrier.newLayout = nextLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	vkCmdPipelineBarrier(
		cmd,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

void vkutil::copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination,
	VkExtent2D srcSize, VkExtent2D destSize)
{
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = destSize.width;
	blitRegion.dstOffsets[1].y = destSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}
