// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vk_types.h"
#include "vk_descriptor.h"
#include "vk_pipelines.h"

#include "../vendor/vma/vk_mem_alloc.h"
#include "../vendor/glm/glm/glm.hpp"


#include <vector>
#include <deque>
#include <array>
#include <functional>

#define DEFAULT_FENCE_TIMEOUT 100000000000
#define WIDTH 1920
#define HEIGHT 1080

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


constexpr unsigned int MAX_CONCURRENT_FRAMES = 2;

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


	// pipelines
	void CreatePipeline();


	void init_imgui();
	void draw_imgui(VkCommandBuffer cmdBuffer, VkImageView targetImageView);

	void SetupDepthStencil();

	struct Vertex
	{
		float position[3];
		float color[3];
	};

	struct VulkanBuffer
	{
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer handle{ VK_NULL_HANDLE };
	};


	VulkanBuffer vertexBuffer;
	VulkanBuffer indexBuffer;
	uint32_t indexCount;

	// uniform buffer block object
	struct UniformBuffer : VulkanBuffer
	{
		// descriptor set stores the resources bound to the binding points in shader
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

		uint8_t* mapped{ nullptr };
	};

	// one ubo per frame, so we can have overframe overlap to be sure uniforms arent updated while still in use
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> _uniformBuffers;


	struct ShaderData
	{
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::mat4 projMatrix;
	};

	// for triangle
	void CreateVertexBuffer();
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
	uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

	// descriptors
	void CreateDescriptors();
	//Descriptors
	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorPool _descriptorPool;
	VkDescriptorSetLayout _drawImageDescriptorLayout;
	void CreateUniformBuffers();
};
