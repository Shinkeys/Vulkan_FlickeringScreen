#pragma once

#include "../headers/types/vk_types.h"

VkInstance CreateInstance();
VkPhysicalDevice ChoosePhysicalDevice(VkPhysicalDevice* physDevices, uint32_t physDevicesCount);
VkDevice CreateDevice(VkPhysicalDevice physDevice, uint32_t familyIndex);