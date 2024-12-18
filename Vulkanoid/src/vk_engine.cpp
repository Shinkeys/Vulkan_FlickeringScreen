#include "../headers/vk_engine.h"

#include <GLFW/glfw3.h>

#include "../vendor/vk-bootstrap/src/VkBootstrap.h"
#include "../headers/vk_images.h"
#include "../headers/vk_types.h"
#include "../headers/vk_initializers.h"
#include "../headers/vk_pipelines.h"

#define VMA_IMPLEMENTATION
#include "../vendor/vma/vk_mem_alloc.h"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_vulkan.h"
#include "../vendor/imgui/imgui_impl_glfw.h"


#include <iostream>
#include <array>
#include <thread>
#include <chrono>
constexpr bool bUseValidationLayers{ true };

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

	init_descriptors();
	
	init_pipelines();

	init_imgui();

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

	// initializing memory allocator
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = _chosenDevice;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_mainDeletionQueue.push_func([&]() 
	{
		vmaDestroyAllocator(_allocator);
	});
}
void VulkanEngine::init_swapchain()
{
	create_swapchain(_windowExtent.width, _windowExtent.height);

	// image size to fit windowSize
	VkExtent3D drawImageExtent
	{
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	// hardcoding 32 bit format of image - RGBA
	_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsageFlags{};
	drawImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info{ vkinit::image_create_info(_drawImage.imageFormat,
		drawImageUsageFlags, drawImageExtent) };

	// allocating image from gpu local memory
	VmaAllocationCreateInfo rimg_allocationInfo{};
	rimg_allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create image
	vmaCreateImage(_allocator, &rimg_info, &rimg_allocationInfo, &_drawImage.image, &_drawImage.allocation, nullptr);

	// image view for the draw to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat,
		_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

	// deletion
	_mainDeletionQueue.push_func([=]()
		{
			vkDestroyImageView(_device, _drawImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
		});
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

	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immediateCommandPool));

	// allocating buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo{ vkinit::command_buffer_allocate_info(_immediateCommandPool, 1) };

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immediateCommandBuffer));

	_mainDeletionQueue.push_func([=]()
		{
			vkDestroyCommandPool(_device, _immediateCommandPool, nullptr);
		});


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

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immediateFence));
	_mainDeletionQueue.push_func([=]()
		{
			vkDestroyFence(_device, _immediateFence, nullptr);
		});
	
}
// swapchain // 
void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{

	vkb::SwapchainBuilder swapchainBuilder{ _chosenDevice, _device, _surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

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

void VulkanEngine::draw_background(VkCommandBuffer cmd)
{
	// making a clear color from frame number. flashing with a 120 frame period
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(_frameNumber / 120.0f));
	clearValue = { {0.0f, 0.0f, flash, 1.0f} };

	VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	// binding the gradient drawing
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

	// binding descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
		_gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);


	// execute the compute pipeline dispatch(we are using 16x16 workgroup)
	vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
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

	_drawExtent.width = _drawImage.imageExtent.width;
	_drawExtent.height = _drawImage.imageExtent.height;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// overwrite general layout
	vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);
	
	// transition img and swapchain img into correct  transfer layouts
	vkutil::transition_image(cmd, _drawImage.image, 
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from draw image to swapchain
	vkutil::copy_image_to_image(cmd, _drawImage.image, 
		_swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

	// set	swapchain img layout to present so we see it on the screen
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// drawing imgui into swapchain
	draw_imgui(cmd, _swapchainImagesView[swapchainImageIndex]);

	// setting swapchain image layout to present
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finalize cmd buffer
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
	bool minimizedWindow = false;

	// Main loop
	while (!bQuit && !glfwWindowShouldClose(_window))
	{
		if (!glfwGetWindowAttrib(_window, GLFW_MAXIMIZED))
		{
			minimizedWindow = true;
		}
		// Poll for and process events
		glfwPollEvents();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		ImGui::Render();
		// Draw function placeholder
		draw();

		// Check if the window should close (like pressing X button)
		if (glfwWindowShouldClose(_window)) {
			bQuit = true;
		}
	}
}

// desciptors

void VulkanEngine::init_descriptors()
{
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes
	{
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
	};

	globalDescriptorAllocator.init_pool(_device, 10, sizes);

	// descriptor set layout for compute draw
	DescriptorLayoutBuilder builder;
	builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	_drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);

	// allocate descriptor set for draw image
	_drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = _drawImage.imageView;

	VkWriteDescriptorSet drawImageWrite{};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext = nullptr;

	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = _drawImageDescriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;


	vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

	// descriptor allocator and new layout should be cleaned up
	_mainDeletionQueue.push_func([&]()
		{
			globalDescriptorAllocator.destroy_pool(_device);

			vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
		});
}

// PIPELINES

void VulkanEngine::init_pipelines()
{
	init_background_pipelines();
}

void VulkanEngine::init_background_pipelines()
{
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

	// SHADERS
	VkShaderModule computeDrawShader;
	if (!vkutil::load_shader_module("shaders/gradient.comp.spv", _device, &computeDrawShader))
	{
		std::cout << "Unable to open your shader by provided path!";
	}

	VkPipelineShaderStageCreateInfo	stageInfo{};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.pNext = nullptr;
	stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.module = computeDrawShader;
	stageInfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageInfo;

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));
	
	vkDestroyShaderModule(_device, computeDrawShader, nullptr);

	_mainDeletionQueue.push_func([&]()
		{
			vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
			vkDestroyPipeline(_device, _gradientPipeline, nullptr);
		});

}


void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(_device, 1, &_immediateFence));
	VK_CHECK(vkResetCommandBuffer(_immediateCommandBuffer, 0));

	VkCommandBuffer cmd = _immediateCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

	// submit command buffer to the queue. render fence will be blocked until the graphics commands finish exec
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediateFence));

	VK_CHECK(vkWaitForFences(_device, 1, &_immediateFence, true, 9999999999));
}

void VulkanEngine::init_imgui()
{
	// creating descriptor pool for imgui
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};


	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// INITIALIZING IMGUI LIB
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(_window, false);

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenDevice;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	// dynamic rendering parameters
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

	// destroying imgui
	_mainDeletionQueue.push_func([=]()
		{
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		});
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmdBuffer, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView,
		nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmdBuffer, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

	vkCmdEndRendering(cmdBuffer);
}