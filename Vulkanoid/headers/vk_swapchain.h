#pragma once

#include "../headers/types/vk_types.h"

#define GLFW_EXPOSE_NATIVE_WIN32
//#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace vkswapchain
{
	VulkanSwapchain CreateSwapchain(VkDevice device, VkSurfaceKHR surface,
		VkSurfaceCapabilitiesKHR surfaceCaps, VkPresentModeKHR presentMode, 
		VkFormat format, VkExtent2D extent, uint32_t familyIndex);
	VkSurfaceKHR CreateWindowSurface(VkInstance instance, GLFWwindow* window);
	VkFormat GetSwapchainSurfaceFormat(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
	VkPresentModeKHR GetSwapchainPresentMode(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
	VkExtent2D GetSwapchainExtent(VkPhysicalDevice physDevice, VkSurfaceKHR surface, GLFWwindow* window);
}