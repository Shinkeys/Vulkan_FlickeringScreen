#include <fstream>
#include "vk_initializers.h"

#include <vector>

namespace vkutil
{
	bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* ourShaderModule);
}