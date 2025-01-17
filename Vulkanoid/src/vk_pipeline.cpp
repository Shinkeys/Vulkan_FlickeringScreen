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


void vkutil::CreateBuffer(VkDevice device, VkPhysicalDevice physDevice, VkDeviceSize deviceSize, VulkanBuffer& bufferData,
	VkBufferUsageFlags usage, VkMemoryMapFlags memFlags)
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
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = cmdPool;
	allocateInfo.commandBufferCount = 1;

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