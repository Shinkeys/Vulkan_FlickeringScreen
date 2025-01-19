#include "../headers/vk_pipelines.h"


VkShaderModule vkutil::LoadShader(const char* filePath,
	VkDevice device)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file)
	{
		throw "Unable to load shaders!";
	}

	// getting file size
	std::size_t fileSize = (std::size_t)file.tellg();


	// buffer should be uint32_t for spirv
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);

	// loading entire file to the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	file.close();

	// creating new shader module
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;


	// codeSize should be in bytes
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	// check if creation goes well
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		std::cout << "Unable to create shader module!";
	}

	
	return shaderModule;
}


void vkutil::CreateImage(VkDevice device, VkPhysicalDevice physDevice, VulkanImage& image, VkImageCreateInfo imageInfo,
	VkMemoryPropertyFlags memFlags)
{
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, image.handle, &memReqs);

	if ((vkCreateImage(device, &imageInfo, nullptr, &image.handle) != VK_SUCCESS))
	{
		throw std::runtime_error("Vk_Pipeline: unable to create image from provided data\n");
	}

	VkPhysicalDeviceMemoryProperties deviceProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceProps);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memReqs.size;
	allocateInfo.memoryTypeIndex = vktool::GetMemoryTypeIndex(deviceProps, memReqs.memoryTypeBits, memFlags);

	if (vkAllocateMemory(device, &allocateInfo, nullptr, &image.memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Vk_Pipeline: unable to allocate  memory for image from provided data\n");
	}

	vkBindImageMemory(device, image.handle, image.memory, 0);
}


void vkutil::CreateBuffer(VkDevice device, VkPhysicalDevice physDevice, VkDeviceSize deviceSize, VulkanBuffer& bufferData,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = deviceSize;
	bufferInfo.usage = usage;
	// could be shared between multiple queues, but dont need now
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &bufferData.handle) != VK_SUCCESS)
	{
		throw std::runtime_error("Vk_Pipeline: unable to create buffer from provided data\n");
	}

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, bufferData.handle, &memReqs);

	VkPhysicalDeviceMemoryProperties deviceProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceProps);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memReqs.size;
	allocateInfo.memoryTypeIndex = vktool::GetMemoryTypeIndex(deviceProps, memReqs.memoryTypeBits,
		memFlags);

	if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferData.memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Vk_Pipeline: unable to allocate from provided data\n");
	}

	vkBindBufferMemory(device, bufferData.handle, bufferData.memory, 0);
}

void vkutil::CopyBuffer(VkDevice device, VkBuffer srcBuffer, 
	VkBuffer dstBuffer, VkDeviceSize size, 
	VkCommandPool cmdPool, VkQueue queue)
{
	VkCommandBufferAllocateInfo allocateInfo{ vkinit::CommandBufferAllocateInfo(cmdPool) };

	VkCommandBuffer  commandBuffer;
	vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{  };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyBuff;
	copyBuff.size = size;
	copyBuff.srcOffset = 0;
	copyBuff.dstOffset = 0;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyBuff);

	vkEndCommandBuffer(commandBuffer);

	SubmitToQueue(device, queue, cmdPool, commandBuffer);
}

void vkutil::CopyBufferToImage(VkDevice device, VkBuffer srcBuffer, VkImage dstImage, 
	VkCommandPool cmdPool, VkExtent3D extent, VkQueue queue)
{
	VkCommandBufferAllocateInfo allocateInfo{ vkinit::CommandBufferAllocateInfo(cmdPool) };

	VkCommandBuffer  commandBuffer;
	vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{  };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferImageCopy  copyBuff;
	copyBuff.bufferOffset = 0;
	copyBuff.bufferRowLength = 0;
	copyBuff.bufferImageHeight = 0;

	copyBuff.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyBuff.imageSubresource.mipLevel = 0;
	copyBuff.imageSubresource.baseArrayLayer = 0;
	copyBuff.imageSubresource.layerCount = 1;

	copyBuff.imageOffset = { 0,0,0 };
	copyBuff.imageExtent = { extent.width, extent.height, extent.depth };


	vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyBuff);

	vkEndCommandBuffer(commandBuffer);

	TransitionImage(commandBuffer, dstImage, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	
	SubmitToQueue(device, queue, cmdPool, commandBuffer);

	vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

void vkutil::TransitionImage(VkCommandBuffer cmd, VkImage image,
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

void vkutil::SubmitToQueue(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkCommandBuffer commandBuffer)
{
	// pushing this buffer to the queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// reminder: someday  would be cool to separate this queue and use transfer queue for operations like that
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
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

std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> vkutil::CreateUniformBuffers(VkDevice device, VkPhysicalDevice physDevice)
{
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = sizeof(ShaderData);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffer;

	VkPhysicalDeviceMemoryProperties deviceMemoryProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceMemoryProps);

	// create the buffers
	for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer[i].handle));

		// get memory requirements 
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, uniformBuffer[i].handle, &memReqs);
		VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = memReqs.size;

		// buffer would be coherent, so dont need to flush every update
		allocInfo.memoryTypeIndex = vktool::GetMemoryTypeIndex(deviceMemoryProps, 
			memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// allocate memory for the uni buff
		VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &uniformBuffer[i].memory));

		// bind memory to buff
		VK_CHECK(vkBindBufferMemory(device,
			uniformBuffer[i].handle, uniformBuffer[i].memory, 0));

		// map buffer once, so can update it without having to map again
		VK_CHECK(vkMapMemory(device, uniformBuffer[i].memory,
			0, sizeof(ShaderData), 0, (void**)&uniformBuffer[i].mapped));
	}

	return uniformBuffer;
}

VkSampler vkutil::CreateSampler(VkDevice device)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 16.0f;


	VkSampler sampler;

	VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));

	return sampler;
}