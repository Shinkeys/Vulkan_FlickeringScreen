// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "types/vk_types.h"
#include "vk_descriptor.h"
#include "vk_pipelines.h"
#include "vk_tools.h"
#include "systems/window.h"
#include "systems/camera.h"
#include "systems/imgui_helper.h"
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

	FrameData& GetCurrentFrame() { return _frames[_frameNumber % MAX_CONCURRENT_FRAMES]; };

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
	ImguiHelper _imguiHelper;

	VulkanoidOperations _operations;
	
	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();

	// swapchain //
	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
	void UpdateSwapchain();

	// pipelines
	void CreatePipeline();

	void PassVulkanStructures();


	void SetupDepthStencil();
	void DestroyDepthStencil();

	struct Vertex
	{
		float position[3];
		float normal[3];
		float texCoord[2];
	};

	Descriptor _globalDescriptor;


	// one ubo per frame, so we can have overframe overlap to be sure uniforms arent updated while still in use
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> _uniformBuffers;


	// for triangle
	void SetupExternalVulkanStructures();
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};

	void CleanBuffers();

	void SetupDescriptor();
};
