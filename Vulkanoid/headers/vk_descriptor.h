#pragma once

#include "vk_types.h"

#include <vector>
#include <span>

class DescriptorLayoutBuilder
{
private:


public:
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	void add_binding(uint32_t binding, VkDescriptorType type);
	void clear();

	VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStageFlags,
		void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

class DescriptorAllocator
{
private:

public:
	struct PoolSizeRatio
	{
		VkDescriptorType type;
		float ratio;
	};

	VkDescriptorPool pool;

	void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void clear_pool(VkDevice device);
	void destroy_pool(VkDevice device);

	VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);


};