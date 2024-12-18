// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vk_types.h"
#include "vk_descriptor.h"

#include "../vendor/vma/vk_mem_alloc.h"



#include <vector>
#include <deque>
#include <functional>


// could make it better later to delete every vulkan handle separately 
struct DeletionQueue
{
	std::deque<std::function<void()>> toDelete;

	void push_func(std::function<void()>&& func)
	{
		toDelete.push_back(func);
	}

	// reverse iterating through "array" because Vulkan works by rule:
	// First in - last out
	void flush()
	{
		for (auto it = toDelete.rbegin(); it != toDelete.rend(); ++it)
		{
			(*it)();
		}

		toDelete.clear();
	}
};

struct FrameData
{
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkFence _renderFence;
	VkSemaphore _renderSemaphore, _swapchainSemaphore;

	DeletionQueue _deletionQueue;
};


constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
	// window settings
	bool _isInitialized{ false };
	int _frameNumber{ 0 };

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct GLFWwindow* _window{ nullptr };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	void draw_background(VkCommandBuffer cmd);

	//run main loop
	void run();
	 

	// setting up commands pool to send it to VkQueue later. VkCommandPool -> VkCommandBuffer -> VkQueue
	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	// VK settings //
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenDevice;
	VkDevice _device; // device for commands
	VkSurfaceKHR _surface; // window surface


	// Swapchain settings //
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImagesView;
	VkExtent2D _swapchainExtent;

	VmaAllocator _allocator;

	// allocation for image
	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;


	//Descriptors
	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	// Pipelines
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

	// other data
	DeletionQueue _mainDeletionQueue;

	// immediate submit structs
	VkFence _immediateFence;
	VkCommandBuffer _immediateCommandBuffer;
	VkCommandPool _immediateCommandPool;
	// immediate submit
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	// swapchain //
	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	// descriptors
	void init_descriptors();

	// pipelines
	void init_pipelines();
	void init_background_pipelines();


	void init_imgui();
	void draw_imgui(VkCommandBuffer cmdBuffer, VkImageView targetImageView);
};
