#pragma once

#include "types/vk_types.h"
#include "vk_resources.h"

#include <vector>
#include <array>
#include <span>



class Descriptor
{
private:	
	VkDevice _device;
	ResourceManager _resources;




	void CreateDescriptorSetLayout(VkDescriptorType descType,
		VkShaderStageFlags flags, uint32_t descriptorCount = 1 /* 1 */);
	[[nodiscard]] VkDescriptorSet AllocateSet(VkDescriptorSetLayout layout);
	[[nodiscard]] VkDescriptorSet AllocateBindlessSet(VkDescriptorSetLayout layout, uint32_t descriptorCount);
	[[nodiscard]]VkDescriptorSetLayout CreateBindlessDescriptorLayout(uint32_t descriptorCount);
	VkDescriptorSetLayout _setLayout;
	VkDescriptorPool _pool;
	VkDescriptorSet _descSet;
public:
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return _setLayout; }
	const VkDescriptorSet GetDescriptorSet() const { return _descSet; }
	void UpdateBindlessBindings(VkDescriptorSet dstSet,
		const std::map<uint32_t, VulkanImage>& images, VkSampler sampler);
	void SetDevice(VkDevice device) { this->_device = device; }
	void Cleanup();
	void DescriptorBasicSetup(uint32_t descriptorCount);
	void UpdateUBOBindings(std::array<UniformBuffer, g_MAX_CONCURRENT_FRAMES> buffer, VkDescriptorSet dstSet);
};