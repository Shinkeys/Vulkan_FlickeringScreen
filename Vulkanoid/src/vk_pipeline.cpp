#include "../headers/vk_pipelines.h"


bool vkutil::load_shader_module(const char* filePath, 
	VkDevice device, VkShaderModule* ourShaderModule)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file)
	{
		std::cout << "Could not open your file!";
		return false;
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
		std::cout << "Error in shader creation!";
		return false;
	}

	*ourShaderModule = shaderModule;
	return true;
}