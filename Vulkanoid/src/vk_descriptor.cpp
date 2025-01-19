#include "../headers/vk_descriptor.h"
#include "../headers/vk_initializers.h"




void Descriptor::ClearPool()
{
	vkResetDescriptorPool(_device, _pool, 0);
}

void Descriptor::DestroyPool()
{
	vkDestroyDescriptorPool(_device, _pool, nullptr);
}

void Descriptor::AllocatePool()
{
	VkDescriptorPoolSize descriptorTypeCounts{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		maxBindlessResources};
	

	VkDescriptorPoolCreateInfo descriptorPoolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCI.poolSizeCount = 1;
	descriptorPoolCI.pPoolSizes = &descriptorTypeCounts;
	descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; // for bindless
	// set max number of descriptor sets that can be requested from this pool
	descriptorPoolCI.maxSets = maxBindlessResources;
	VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCI, nullptr, &_pool));
}

void Descriptor::CreateBindlessDescriptorLayout()
{
	VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | // could be used only for last binding in set layout,
			// means that is variable sized binding of desc, which size would be specified during allocation
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBinding.descriptorCount = maxBindlessResources;
		layoutBinding.binding = 1;

		layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
		};
		descriptorLayoutCI.bindingCount = 1;
		descriptorLayoutCI.pBindings = &layoutBinding;
		descriptorLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

		VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &_setLayout));
}

void Descriptor::CreateDescriptorSetLayout(VkDescriptorType descType,
	VkShaderStageFlags flags, uint32_t descriptorCount /* 1 */)
{
	// define interface between application and shader
	// connect different shader stages to descriptors for binding uniform buffers
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.descriptorType = descType;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = flags;

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
	};
	descriptorLayoutCI.bindingCount = 1;
	descriptorLayoutCI.pBindings = &layoutBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &_setLayout));
}

VkDescriptorSet Descriptor::AllocateBindlessSet(VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
	uint32_t maxBinding = maxBindlessResources - 1;
	countInfo.descriptorSetCount = 1;
	// max allocate number
	countInfo.pDescriptorCounts = &maxBinding;

	allocInfo.pNext = &countInfo;

	VkDescriptorSet descriptorSet;

	VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet));

	return descriptorSet;
}



VkDescriptorSet Descriptor::AllocateSet(VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet descriptorSet;
	VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet));

	return descriptorSet;
}

void Descriptor::UpdateShaderBindingPoints(VulkanBuffer buffer, VkDescriptorSet dstSet, std::vector<VulkanImage>& images, uint32_t texturePathsSize)
{
	// update descriptor set determining the shader binding points
		// for every binding point used in a shader there needs to be one
		// desc matching that binding point
	VkWriteDescriptorSet writeDescSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer.handle;
	bufferInfo.range = sizeof(ShaderData);

	// binding 0 - ubo
	writeDescSet.dstSet = dstSet;
	writeDescSet.descriptorCount = 1;
	writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescSet.pBufferInfo = &bufferInfo;
	writeDescSet.dstBinding = 0;
	vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, VK_NULL_HANDLE);

	for (uint32_t i = 0; i < texturePathsSize; ++i)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler; // to do
		imageInfo.imageView = images[i].view;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


		VkWriteDescriptorSet writeDescSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeDescSet.dstSet = dstSet;
		writeDescSet.dstBinding = 1;
		writeDescSet.dstArrayElement = i;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescSet.pImageInfo = &imageInfo;
		
	}
}

