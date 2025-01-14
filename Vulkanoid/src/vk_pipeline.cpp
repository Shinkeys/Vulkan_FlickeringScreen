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