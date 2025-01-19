#pragma once

#include "vk_types.h"

#include <vector>
#include <array>
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
	[[nodiscard]] VkDescriptorSet AllocateSet(VkDescriptorSetLayout layout);
	[[nodiscard]] VkDescriptorSet AllocateBindlessSet(VkDescriptorSetLayout layout);
	[[nodiscard]]VkDescriptorSetLayout CreateBindlessDescriptorLayout();
	VkDescriptorSetLayout _setLayout;
	VkDescriptorPool _pool;
	VkDescriptorSet _descSet;
public:
	const VkDescriptorSetLayout GetDescriptorSetLayout() const { return _setLayout; }
	const VkDescriptorSet GetDescriptorSet() const { return _descSet; }
	void UpdateBindlessBindings(VkDescriptorSet dstSet,
		std::vector<VulkanImage> images, uint32_t texturePathsSize, VkSampler sampler);
	void SetDevice(VkDevice device) { this->_device = device; }
	void Cleanup();
	void DescriptorBasicSetup();
	void UpdateUBOBindings(std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> buffer, VkDescriptorSet dstSet);
};