#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

namespace vktool
{
	VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physDevice, VkFormat *depthFormat);
}