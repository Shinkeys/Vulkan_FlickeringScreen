#include "../headers/vk_engine.h"

#include <GLFW/glfw3.h>

#include "../vendor/vk-bootstrap/src/VkBootstrap.h"
#include "../headers/vk_images.h"
#include "../headers/vk_types.h"
#include "../headers/vk_initializers.h"
#include <iostream>
#include <array>
#include <thread>
#include <chrono>
constexpr bool bUseValidationLayers{ true };


// macro to check for Vulkan iteractions errors
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err{x};                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)



void VulkanEngine::init()
{
	// Initialize GLFW
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	// Specify that we want to use Vulkan with GLFW
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Create a windowed mode window and its OpenGL context
	_window = glfwCreateWindow(
		_windowExtent.width,
		_windowExtent.height,
		"Vulkan Engine",
		nullptr,
		nullptr
	);


	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

	if (!_window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	// Everything went fine
	_isInitialized = true;
}

void VulkanEngine::init_vulkan()
{
	vkb::InstanceBuilder builder;

	auto inst_ret = builder.set_app_name("Vulkan learning")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();



	vkb::Instance vkb_inst = inst_ret.value();

	// grab instance
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;


	////////
	glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);
	
	// enabling features from vk 1.3
	VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;
	
	// enabling features from vk 1.2
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;


	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physDevice{ selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(_surface)
		.select()
		.value() };

	// final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physDevice };


	vkb::Device vkbDevice{ deviceBuilder.build().value() };

	// get the vkdevice handle used in the rest of vulkan application
	_device = vkbDevice.device;
	_chosenDevice = physDevice.physical_device;


	// getting a graphics queue with vk bootstrap
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}
void VulkanEngine::init_swapchain()
{
	create_swapchain(_windowExtent.width, _windowExtent.height);
}
void VulkanEngine::init_commands()
{
	// creating a command pool for commands submitted to the graphics queue
	VkCommandPoolCreateInfo	commandPoolInfo{ vkinit::command_pool_create_info(_graphicsQueueFamily,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) };

	for (int i = 0; i < FRAME_OVERLAP; ++i)
	{
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		// allocate the default command buffer that will be used for render
		VkCommandBufferAllocateInfo cmdAllocInfo{ vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1) };


		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
	}
}
void VulkanEngine::init_sync_structures()
{
	// 1 fence to control when gpu finished rendering the frame,
	// and 2 semaphores to synchronize rendering with swapchain

	VkFenceCreateInfo fenceCreateInfo{ vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT) };
	VkSemaphoreCreateInfo semaphoreCreateInfo{ vkinit::semaphore_create_info() };
	/////////////////////////////////////////////////////////////////
	//////////// CHECK SEMAPHORE CODE LATER /////////////////////////
	////////////////////////////////////////////////////////////////

	for (int i = 0; i < FRAME_OVERLAP; ++i)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

	}
	
}
// swapchain // 
void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{

	vkb::SwapchainBuilder swapchainBuilder{ _chosenDevice, _device, _surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8_UNORM;

	vkb::Swapchain vkbSwapChain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, 
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swapchainExtent = vkbSwapChain.extent;
	// store swapchain and its related images
	_swapchain = vkbSwapChain.swapchain;
	_swapchainImages = vkbSwapChain.get_images().value();
	_swapchainImagesView = vkbSwapChain.get_image_views().value();


}

void VulkanEngine::destroy_swapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	//destroy swapchain resources
	for (int i = 0; i < _swapchainImagesView.size(); ++i)
	{
		vkDestroyImageView(_device, _swapchainImagesView[i], nullptr);
	}

}


void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		vkDeviceWaitIdle(_device);


		_mainDeletionQueue.flush();
		for (int i = 0; i < FRAME_OVERLAP; i++) {

			//already written from before
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);

			_frames[i]._deletionQueue.flush();
		}



		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

		vkDestroyInstance(_instance, nullptr);

		// destroying window
		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}



void VulkanEngine::draw()
{
	// waiting until gpu has finished rendering last frame. 1 sec between
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000  )); // 1 bill

	get_current_frame()._deletionQueue.flush();


	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	// requesting image from swapchain
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, 
		get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));


	VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	// now we ensure that the commands finished executing, we can safely reset command buffer
	VK_CHECK(vkResetCommandBuffer(cmd, 0));


	// begin the command buffer recording
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// starting command buffer recording	
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// making swapchain image into writable mode
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// make a clear color from number. will flash with a 120s period
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(_frameNumber / 120.0f));
	clearValue = { {0.0f, 0.0f, flash, 1.0f} };

	VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	// clear image
	vkCmdClearColorImage(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	// make swapchain image into presentable mode
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finalize the command buffer
	VK_CHECK(vkEndCommandBuffer(cmd));

	//prepare the submission to the queue. 
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

	VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

	//prepare present
	// this will put the image we just rendered to into the visible window.
	// we want to wait on the _renderSemaphore for that, 
	// as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	//increase the number of frames drawn
	_frameNumber++;
		
}

void VulkanEngine::run()
{
	bool bQuit = false;

	// Main loop
	while (!bQuit && !glfwWindowShouldClose(_window))
	{
		// Poll for and process events
		glfwPollEvents();

		// Draw function placeholder
		draw();

		// Check if the window should close (like pressing X button)
		if (glfwWindowShouldClose(_window)) {
			bQuit = true;
		}
	}
}
