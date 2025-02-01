#pragma once

#include "../types/vk_types.h"
#include "../vk_initializers.h"
#include "window.h"

class ImguiHelper
{
private:
	VkDevice _device;
	VkInstance _instance;
	VkPhysicalDevice _physDevice;
	VkQueue _graphicsQueue;


	VkDescriptorPool _imguiPool;

	void ImguiSettings();
	void ImguiWindows();
public:
	void InitImgui(GLFWwindow* window, VkFormat swapchainImgFormat);
	void DrawImgui(VkCommandBuffer cmdBuffer, VkImageView targetImageView, VkExtent2D swapchainExtent);
	void DestroyImgui();
	void Update();
	void SetStructures(VkDevice device, VkInstance instance, VkPhysicalDevice physDevice, VkQueue graphicsQueue);
};