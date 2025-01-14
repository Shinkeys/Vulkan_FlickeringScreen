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