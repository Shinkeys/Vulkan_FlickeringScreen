#include "../headers/vk_descriptor.h"
#include "../headers/vk_initializers.h"


void Descriptor::DescriptorBasicSetup(uint32_t descriptorCount)
{
	_setLayout = CreateBindlessDescriptorLayout(descriptorCount);
	_descSet = AllocateBindlessSet(_setLayout, descriptorCount);
}

void Descriptor::Cleanup()
{
	_resources.FlushQueue();
}

VkDescriptorSetLayout Descriptor::CreateBindlessDescriptorLayout(uint32_t descriptorCount)
{
	VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT; // could be used only for last binding in set layout,
		// means that it is a variable binding of desc, which size would be specified during allocation

	std::array<VkDescriptorSetLayoutBinding, 2> layoutBinding{};
	layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding[0].descriptorCount = 1;
	layoutBinding[0].binding = 0;
	layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding[1].descriptorCount = descriptorCount;
	layoutBinding[1].binding = 1;	
	layoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
	};
	descriptorLayoutCI.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	descriptorLayoutCI.pBindings = layoutBinding.data();
	descriptorLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	VkDescriptorSetLayout setLayout;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &setLayout));

	_resources.PushFunction([=]
		{
			vkDestroyDescriptorSetLayout(_device, setLayout, nullptr);
		});

	return setLayout;
}

VkDescriptorSet Descriptor::AllocateBindlessSet(VkDescriptorSetLayout layout, uint32_t descriptorCount)
{
	VkDescriptorPoolSize descriptorTypeCounts[]{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, // bind 0
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount} }; // bind 1

	VkDescriptorPoolCreateInfo descriptorPoolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCI.poolSizeCount = 2;
	descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
	descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; // for bindless
	// set max number of descriptor sets that can be requested from this pool
	descriptorPoolCI.maxSets = 1;
	VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCI, nullptr, &_pool));


	_resources.PushFunction([=]
		{
			vkDestroyDescriptorPool(_device, _pool, nullptr);
		});

	VkDescriptorSetAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
	countInfo.descriptorSetCount = 1;
	// max allocate number
	countInfo.pDescriptorCounts = &descriptorCount;

	allocInfo.pNext = &countInfo;

	VkDescriptorSet descriptorSet;

	VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet));

	return descriptorSet;
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

	_resources.PushFunction([=]
		{
			vkDestroyDescriptorSetLayout(_device, _setLayout, nullptr);
		});
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

void Descriptor::UpdateUBOBindings(std::array<UniformBuffer, g_MAX_CONCURRENT_FRAMES> buffer, VkDescriptorSet dstSet)
{
	// update descriptor set determining the shader binding points
		// for every binding point used in a shader there needs to be one
		// desc matching that binding point

	for (uint32_t i = 0; i < g_MAX_CONCURRENT_FRAMES; ++i)
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
		writeDescSet.dstBinding = 0;
		vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, VK_NULL_HANDLE);
	}
}


// to do: refactor
void Descriptor::UpdateBindlessBindings(VkDescriptorSet dstSet, 
	const std::map<uint32_t, VulkanImage>& images, VkSampler sampler)
{
	assert(images.size() > 0, "Unordered map of textures is empty!");
	assert(sampler != VK_NULL_HANDLE, "Sampler is not created!");

	uint32_t i = 1; // 0 is empty texture
	for (const auto& image : images)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = sampler;
		imageInfo.imageView = image.second.view;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


	
		VkWriteDescriptorSet writeDescSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeDescSet.dstSet = dstSet;
		writeDescSet.dstBinding = 1;
		writeDescSet.dstArrayElement = i;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescSet.pImageInfo = &imageInfo;
		
		vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, VK_NULL_HANDLE);

		++i;
	}
}

