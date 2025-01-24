// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "types/vk_types.h"
#include "vk_descriptor.h"
#include "vk_pipelines.h"
#include "vk_tools.h"
#include "window.h"
#include "systems/camera.h"
#include "vk_mesh.h"





#include <vector>
#include <deque>
#include <array>
#include <functional>

#define DEFAULT_FENCE_TIMEOUT 100000000000




struct FrameData
{
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkFence _renderFence;
	VkSemaphore _renderSemaphore, _swapchainSemaphore;

};

struct ImmediateData
{
	// immediate submit structs
	VkFence _immediateFence;
	VkCommandBuffer _immediateCommandBuffer;
	VkCommandPool _immediateCommandPool;
};



class VulkanEngine {
public:
	// window settings
	bool _isInitialized{ false };


	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
	 

	// setting up commands pool to send it to VkQueue later. VkCommandPool -> VkCommandBuffer -> VkQueue
	FrameData _frames[MAX_CONCURRENT_FRAMES];

	FrameData& get_current_frame() { return _frames[_frameNumber % MAX_CONCURRENT_FRAMES]; };

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

	VkFormat _depthFormat;
	DepthStencil _depthStencil;

	
	// Pipelines
	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;

	ImmediateData _immediate;

	// immediate submit
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
	ResourceManager _resources;
	std::unique_ptr<Window> _windowManager = std::make_unique<Window>();
	Camera _camera{ _windowManager.get()};
	VulkanMesh _mesh;
	
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	// swapchain //
	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	// pipelines
	void CreatePipeline();

	void PassVulkanStructures();

	void InitImgui();
	void DrawImgui(VkCommandBuffer cmdBuffer, VkImageView targetImageView);
	void DestroyImgui();
	VkDescriptorPool _imguiPool;

	void SetupDepthStencil();

	struct Vertex
	{
		float position[3];
		float normal[3];
		float texCoord[2];
	};

	Descriptor _globalDescriptor;

	uint32_t indexCount;

	// one ubo per frame, so we can have overframe overlap to be sure uniforms arent updated while still in use
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> _uniformBuffers;


	// for triangle
	void SetupExternalVulkanStructures();
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};

	void CleanBuffers();

	void SetupDescriptor();
};
