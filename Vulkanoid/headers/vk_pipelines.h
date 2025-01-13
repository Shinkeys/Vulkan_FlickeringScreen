#include <fstream>
#include "vk_initializers.h"

#include <vector>

namespace vkutil
{
	VkShaderModule LoadShader(const char* filePath, VkDevice device);
}