#pragma once

#include "vk_types.h"

#include <vector>
#include <span>



class Descriptor
{
private:	
	VkDevice _device;




	// to do it later 
	const uint32_t maxBindlessResources = 16536;


	void CreateDescriptorSetLayout(VkDescriptorType descType,
		VkShaderStageFlags flags, uint32_t descriptorCount = 1 /* 1 */);
	void AllocatePool();
	VkDescriptorSet AllocateSet(VkDescriptorSetLayout layout);
	void UpdateShaderBindingPoints(VulkanBuffer buffer, VkDescriptorSet dstSet);
	VkDescriptorSet AllocateBindlessSet(VkDescriptorSetLayout layout);
	void CreateBindlessDescriptorLayout();
public:
	Descriptor(VkDevice device) : _device{ device }
	{

	}
	VkDescriptorPool _pool;
	VkDescriptorSetLayout _setLayout;
	void ClearPool();
	void DestroyPool();
};