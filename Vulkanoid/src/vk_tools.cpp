#include "../headers/vk_tools.h"

VkBool32 vktool::GetSupportedDepthFormat(VkPhysicalDevice physDevice, VkFormat* depthFormat)
{
	// all depth formats might be optional so we need to find a suitable
	std::vector<VkFormat> formatList = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
	};

	for (const auto& x : formatList)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physDevice, x, &formatProps);
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			*depthFormat = x;
			return true;
		}
	}

	return false;
}

// request device memory type that support all the property flags we need
uint32_t vktool::GetMemoryTypeIndex(VkPhysicalDeviceMemoryProperties memoryProps, uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < memoryProps.memoryTypeCount; ++i)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProps.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		typeBits >>= 1;
	}
	throw "Unable to find a suitable memory type";
}

uint32_t vktool::GetGraphicsFamilyIndex(VkPhysicalDevice physDevice)
{
	uint32_t queueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());
	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			return i;
		}
	}

	return VK_QUEUE_FAMILY_IGNORED;
}