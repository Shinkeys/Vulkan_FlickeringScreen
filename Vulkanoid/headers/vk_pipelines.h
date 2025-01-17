#include <fstream>
#include "vk_initializers.h"
#include "vk_tools.h"
#include "vk_types.h"

#include <vector>

namespace vkutil
{
	VkShaderModule LoadShader(const char* filePath, VkDevice device);

	void CreateBuffer(VkDevice device, VkPhysicalDevice physDevice, 
		VkDeviceSize deviceSize, VulkanBuffer& bufferData,
		VkBufferUsageFlags usage, VkMemoryMapFlags memFlags);

	void CopyBuffer(VkDevice device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool cmdPool, VkQueue queue);
}