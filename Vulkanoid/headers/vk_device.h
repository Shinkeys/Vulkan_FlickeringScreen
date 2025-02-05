#pragma once

#include "../headers/types/vk_types.h"

namespace vkdevice
{
	VkInstance CreateInstance();
	VkPhysicalDevice ChoosePhysicalDevice(VkPhysicalDevice* physDevices, uint32_t physDevicesCount);
	VkDevice CreateDevice(VkPhysicalDevice physDevice, uint32_t familyIndex);
}

namespace vkdebug
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	VkDebugUtilsMessengerEXT CreateDebugCallback(VkInstance instance);
	void DestroyDebugCallback(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
	void DebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info);
}