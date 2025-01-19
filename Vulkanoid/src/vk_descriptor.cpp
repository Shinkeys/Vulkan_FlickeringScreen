#include "../headers/vk_descriptor.h"
#include "../headers/vk_initializers.h"


void Descriptor::DescriptorBasicSetup()
{
	AllocatePool();
	_setLayout = CreateBindlessDescriptorLayout();
	_descSet = AllocateBindlessSet(_setLayout);
}


void Descriptor::Cleanup()
{
	vkDestroyDescriptorPool(_device, _pool, nullptr);
}

void Descriptor::AllocatePool()
{
	VkDescriptorPoolSize descriptorTypeCounts[]{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxBindlessResources},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1} };
	

	VkDescriptorPoolCreateInfo descriptorPoolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCI.poolSizeCount = 1;
	descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
	descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; // for bindless
	// set max number of descriptor sets that can be requested from this pool
	descriptorPoolCI.maxSets = maxBindlessResources;
	VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCI, nullptr, &_pool));
}

VkDescriptorSetLayout Descriptor::CreateBindlessDescriptorLayout()
{
	VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT; // could be used only for last binding in set layout,
		// means that it is a variable binding of desc, which size would be specified during allocation

	std::array<VkDescriptorSetLayoutBinding, 2> layoutBinding{};
	layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding[0].descriptorCount = maxBindlessResources;
	layoutBinding[0].binding = 0;
	layoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	
	layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding[1].descriptorCount = 1;
	layoutBinding[1].binding = 1;
	layoutBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
	};
	descriptorLayoutCI.bindingCount = 1;
	descriptorLayoutCI.pBindings = layoutBinding.data();
	descriptorLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	VkDescriptorSetLayout setLayout;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &setLayout));

	return setLayout;
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

void Descriptor::UpdateUBOBindings(std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> buffer, VkDescriptorSet dstSet)
{
	// update descriptor set determining the shader binding points
		// for every binding point used in a shader there needs to be one
		// desc matching that binding point

	for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		VkWriteDescriptorSet writeDescSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer[i].handle;
		bufferInfo.range = sizeof(ShaderData);

		// binding 0 - ubo
		writeDescSet.dstSet = dstSet;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescSet.pBufferInfo = &bufferInfo;
		writeDescSet.dstBinding = 1;
		vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, VK_NULL_HANDLE);
	}
}


// to do: refactor
void Descriptor::UpdateBindlessBindings(VkDescriptorSet dstSet, 
	std::vector<VulkanImage> images, uint32_t texturePathsSize, VkSampler sampler)
{
	for (uint32_t i = 0; i < texturePathsSize; ++i)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = sampler;
		imageInfo.imageView = images[i].view;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


		VkWriteDescriptorSet writeDescSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeDescSet.dstSet = dstSet;
		writeDescSet.dstBinding = 0;
		writeDescSet.dstArrayElement = i;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescSet.pImageInfo = &imageInfo;
		
		vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, VK_NULL_HANDLE);
	}
}

