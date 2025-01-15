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
	_windowManager.InitializeGLFW();

	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

	CreateVertexBuffer();

	CreateUniformBuffers();

	CreateDescriptors();
	
	CreatePipeline();

	init_imgui();


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

	_windowManager.CreateWindowSurface(_instance, &_surface);
	
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


	vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceMemoryProperties);

	// final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physDevice };


	vkb::Device vkbDevice{ deviceBuilder.build().value() };

	// get the vkdevice handle used in the rest of vulkan application
	_device = vkbDevice.device;
	_chosenDevice = physDevice.physical_device;
	

	// depth
	vktool::GetSupportedDepthFormat(physDevice, &_depthFormat);
	SetupDepthStencil();
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

}
void VulkanEngine::init_swapchain()
{
	create_swapchain(_windowManager.GetWindowWidth(), _windowManager.GetWindowHeight());

	// image size to fit windowSize
	VkExtent3D drawImageExtent
	{
		_windowManager.GetWindowWidth(),
		_windowManager.GetWindowHeight(),
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

}
void VulkanEngine::init_commands()
{
	// creating a command pool for commands submitted to the graphics queue
	VkCommandPoolCreateInfo	commandPoolInfo{ vkinit::command_pool_create_info(_graphicsQueueFamily,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) };

	for (int i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		// allocate the default command buffer that will be used for render
		VkCommandBufferAllocateInfo cmdAllocInfo{ vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1) };


		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
	}

	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immediate._immediateCommandPool));

	// allocating buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo{ vkinit::command_buffer_allocate_info(_immediate._immediateCommandPool, 1) };

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immediate._immediateCommandBuffer));

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

	for (int i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

	}

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immediate._immediateFence));
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

void VulkanEngine::CleanBuffers()
{
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	vkDestroyDescriptorPool(_device, globalDescriptorAllocator.pool, nullptr);
	for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		vkDestroyBuffer(_device, _uniformBuffers[i].handle, nullptr);
		vkFreeMemory(_device, _uniformBuffers[i].memory, nullptr);
	}

	// VERTEX INDEX BUFFERS
	vkDestroyBuffer(_device, vertexBuffer.handle, nullptr);
	vkFreeMemory(_device, vertexBuffer.memory, nullptr);
	vkDestroyBuffer(_device, indexBuffer.handle, nullptr);
	vkFreeMemory(_device, indexBuffer.memory, nullptr);
}

void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		vkDeviceWaitIdle(_device);

		// destroy immediate data
		vkDestroyCommandPool(_device, _immediate._immediateCommandPool, nullptr);
		vkDestroyFence(_device, _immediate._immediateFence, nullptr);

		for (int i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			//already written from before
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
		}

		vkDestroyImage(_device, _drawImage.image, nullptr);
		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
		vkDestroyImage(_device, _depthStencil.image, nullptr);
		vkFreeMemory(_device, _depthStencil.memory, nullptr);
		vkDestroyImageView(_device, _depthStencil.view, nullptr);

		CleanBuffers();
		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

		vkDestroyInstance(_instance, nullptr);

		// destroying window
		glfwDestroyWindow(_windowManager.GetWindowInstance());
		glfwTerminate();
	}
}

void VulkanEngine::draw()
{
	_camera.Update();
	// waiting until gpu has finished rendering last frame. 1 sec between
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, VK_TRUE, UINT64_MAX)); 

	uint32_t imageIndex{};

	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, 
		get_current_frame()._swapchainSemaphore, nullptr, &imageIndex));

	// update uniform buffer for the next frame
	ShaderData shaderData{};
	glm::mat4 projection = glm::perspective(90.0f, static_cast<float>(_windowManager.GetWindowWidth()) /
		static_cast<float>(_windowManager.GetWindowHeight()), 0.1f, 100.0f);
	glm::mat4 view = _camera.GetLookToMatrix();
	glm::mat4 model = glm::mat4(1.0f);

	shaderData.projMatrix = projection;
	shaderData.viewMatrix = view;
	shaderData.modelMatrix = model;

	// copy matrices to the current frame UBO
	memcpy(_uniformBuffers[_frameNumber].mapped, &shaderData, sizeof(ShaderData));


	VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	// now we ensure that the commands finished executing, we can safely reset command buffer
	VK_CHECK(vkResetCommandBuffer(cmd, 0));


	// begin the command buffer recording
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// for background IMAGE
	//_drawExtent.width = _drawImage.imageExtent.width;
	//_drawExtent.height = _drawImage.imageExtent.height;

	//// with dynamic rendering need to explicitly add layout transition by using barries
	//vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	////draw_background(cmd);
	/*vkutil::transition_image(cmd, _drawImage.image, 
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);*/
	

	// transition img and swapchain img into correct  transfer layouts
	vkutil::transition_image(cmd, _swapchainImages[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	// transition image for depth
	vkutil::transition_image(cmd, _depthStencil.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });


	// define attachments used in dynamic rendering
	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = _swapchainImagesView[imageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.2f, 0.0f };
	// Depth/stencil attachment
	VkRenderingAttachmentInfo depthStencilAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthStencilAttachment.imageView = _depthStencil.view;
	depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
	renderingInfo.renderArea = { 0, 0, _windowManager.GetWindowWidth(), _windowManager.GetWindowHeight() };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;
	renderingInfo.pStencilAttachment = &depthStencilAttachment;




	//// execute a copy from draw image to swapchain
	//vkutil::copy_image_to_image(cmd, _drawImage.image, 
	//	_swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

	//// set	swapchain img layout to present so we see it on the screen
	//vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
	//	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// drawing imgui into swapchain
	//draw_imgui(cmd, _swapchainImagesView[swapchainImageIndex]);

	// Start a dynamic rendering section
	vkCmdBeginRendering(cmd, &renderingInfo);
	// Update dynamic viewport state
	VkViewport viewport{ 0.0f, 0.0f, (float)_windowManager.GetWindowWidth(),
		(float)_windowManager.GetWindowHeight(), 0.0f, 1.0f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	// Update dynamic scissor state
	VkRect2D scissor{ 0, 0, _windowManager.GetWindowWidth(), _windowManager.GetWindowHeight() };
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	// Bind descriptor set for the current frame's uniform buffer, so the shader uses the data from that buffer for this draw
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_uniformBuffers[_frameNumber].descriptorSet, 0, nullptr);
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1]{ 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.handle, offsets);
	// Bind triangle index buffer
	vkCmdBindIndexBuffer(cmd, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
	// Draw indexed triangle
	vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
	// Finish the current dynamic rendering section
	vkCmdEndRendering(cmd);


	// setting swapchain image layout to present
	vkutil::transition_image(cmd, _swapchainImages[imageIndex],
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

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

	presentInfo.pImageIndices = &imageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	//increase the number of frames drawn
	_frameNumber = (_frameNumber + 1) % MAX_CONCURRENT_FRAMES;
		
	
}

void VulkanEngine::run()
{
	bool bQuit = false;
	bool minimizedWindow = false;

	// Main loop
	while (!bQuit && !glfwWindowShouldClose(_windowManager.GetWindowInstance()))
	{
		if (!glfwGetWindowAttrib(_windowManager.GetWindowInstance(), GLFW_MAXIMIZED))
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
		if (glfwWindowShouldClose(_windowManager.GetWindowInstance())) {
			bQuit = true;
		}
	}
}


// PIPELINES


void VulkanEngine::CreatePipeline()
{
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;
	VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_pipelineLayout));


	VkGraphicsPipelineCreateInfo pipelineCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCI.layout = _pipelineLayout;

	// describes how primitives are assembled
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCI.depthClampEnable = VK_FALSE;
	rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCI.depthBiasEnable = VK_FALSE;
	rasterizationStateCI.lineWidth = 1.0f;


	// color blend state describes how blend factors are calculated
	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = 0xf;
	blendAttachmentState.blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo colorBlendStateCI{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendStateCI.attachmentCount = 1;
	colorBlendStateCI.pAttachments = &blendAttachmentState;

	// number of viewports and scissors(overriden by dynamic state)
	VkPipelineViewportStateCreateInfo viewportStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;

	// dynamic states
	std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

	// depth stencil state(TO MAKE METHOD WITH INITIALIZATION LATER)
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCI.stencilTestEnable = VK_FALSE;
	depthStencilStateCI.front = depthStencilStateCI.back;

	// doesnt use multi sampling but should pass it anyway
	VkPipelineMultisampleStateCreateInfo multisampleStateCI{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// VERTEX INPUT binding
	VkVertexInputBindingDescription vertexInputBinding{};
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(Vertex);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// input attribute bindings describe shader attribute location and memory layouts
	VkVertexInputAttributeDescription vertexInputAttributs{};
	// layout (location = 0) in vec3 inPos;
	// layout (location = 1) in vec3 inColor;

	// position
	vertexInputAttributs.binding = 0;
	vertexInputAttributs.location = 0;

	// pos attribute is 3 32 bit signed float
	vertexInputAttributs.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs.offset = offsetof(Vertex, position);


	// vertex input state for pipeline creation
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputStateCI.vertexBindingDescriptionCount = 1;
	vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputStateCI.vertexAttributeDescriptionCount = 1;
	vertexInputStateCI.pVertexAttributeDescriptions = &vertexInputAttributs;
	

	// shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

	// vertex shader
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vkutil::LoadShader("shaders/main_vert.spv", _device);
	shaderStages[0].pName = "main";
	assert(shaderStages[0].module != VK_NULL_HANDLE);

	// fragment shader
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = vkutil::LoadShader("shaders/main_frag.spv", _device);
	shaderStages[1].pName = "main";
	assert(shaderStages[1].module != VK_NULL_HANDLE);
	
	// set pipeline shader stage info
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();

	// attachment info for dynamic rendering
	VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
	pipelineRenderingCI.colorAttachmentCount = 1;
	pipelineRenderingCI.pColorAttachmentFormats = &_swapchainImageFormat;
	pipelineRenderingCI.depthAttachmentFormat = _depthFormat;
	pipelineRenderingCI.stencilAttachmentFormat = _depthFormat;

	// assign the pipeline states to the pipeline creation info struct
	pipelineCI.pVertexInputState = &vertexInputStateCI;
	pipelineCI.pInputAssemblyState = &inputAssemblyCI;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.pColorBlendState = &colorBlendStateCI;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pNext = &pipelineRenderingCI;

	// IMPORTANT TO MAKE PIPELINE CACHE LATER
	VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &_pipeline));

	vkDestroyShaderModule(_device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(_device, shaderStages[1].module, nullptr);

}

void VulkanEngine::SetupDepthStencil()
{
	// Create an optimal tiled image used as the depth stencil attachment
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = _depthFormat;
	imageCI.extent = { _windowManager.GetWindowWidth(), _windowManager.GetWindowHeight(), 1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK(vkCreateImage(_device, &imageCI, nullptr, &_depthStencil.image));

	// Allocate memory for the image (device local) and bind it to our image
	VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(_device, _depthStencil.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(_device, &memAlloc, nullptr, &_depthStencil.memory));
	VK_CHECK(vkBindImageMemory(_device, _depthStencil.image, _depthStencil.memory, 0));

	// Create a view for the depth stencil image
	// Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
	// This allows for multiple views of one image with differing ranges (e.g. for different layers)
	VkImageViewCreateInfo depthStencilViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	depthStencilViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilViewCI.format = _depthFormat;
	depthStencilViewCI.subresourceRange = {};
	depthStencilViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT)
	if (_depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
		depthStencilViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	depthStencilViewCI.subresourceRange.baseMipLevel = 0;
	depthStencilViewCI.subresourceRange.levelCount = 1;
	depthStencilViewCI.subresourceRange.baseArrayLayer = 0;
	depthStencilViewCI.subresourceRange.layerCount = 1;
	depthStencilViewCI.image = _depthStencil.image;
	VK_CHECK(vkCreateImageView(_device, &depthStencilViewCI, nullptr, &_depthStencil.view));
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(_device, 1, &_immediate._immediateFence));
	VK_CHECK(vkResetCommandBuffer(_immediate._immediateCommandBuffer, 0));

	VkCommandBuffer cmd = _immediate._immediateCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

	// submit command buffer to the queue. render fence will be blocked until the graphics commands finish exec
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediate._immediateFence));

	VK_CHECK(vkWaitForFences(_device, 1, &_immediate._immediateFence, true, DEFAULT_FENCE_TIMEOUT));

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

	ImGui_ImplGlfw_InitForVulkan(_windowManager.GetWindowInstance(), false);

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

void VulkanEngine::CreateVertexBuffer()
{
	const std::vector<Vertex> vertices{
		{-1.0f, -1.0f, -1.0f}, // 0
		{ 1.0f, -1.0f, -1.0f}, // 1
		{ 1.0f,  1.0f, -1.0f}, // 2
		{-1.0f,  1.0f, -1.0f}, // 3
		{-1.0f, -1.0f,  1.0f}, // 4
		{ 1.0f, -1.0f,  1.0f}, // 5
		{ 1.0f,  1.0f,  1.0f}, // 6 
		{-1.0f,  1.0f,  1.0f}  // 7
	};

	const uint32_t vertexBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);

	std::vector<uint32_t> indices{
	 0, 1, 3, 3, 1, 2,
	1, 5, 2, 2, 5, 6,
	5, 4, 6, 6, 4, 7,
	4, 0, 7, 7, 0, 3,
	3, 2, 7, 7, 2, 6,
	4, 5, 0, 0, 5, 1
	};
	indexCount = static_cast<uint32_t>(indices.size());
	const uint32_t indexSize = indexCount * sizeof(uint32_t);

	VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkMemoryRequirements memoryReqs;

	// to effectively access it on the gpu better to use staging buffer(basically intermediate memory)
	VulkanBuffer stagingBuffer;
	VkBufferCreateInfo stagingBufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingBufferCI.size = indexSize + vertexBufferSize;
	// using buffer as copy SOURCE
	stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	// creating host visible buffer to copy the vertex data to staging buff
	VK_CHECK(vkCreateBuffer(_device, &stagingBufferCI, nullptr, &stagingBuffer.handle));
	vkGetBufferMemoryRequirements(_device, stagingBuffer.handle, &memoryReqs);
	memAlloc.allocationSize = memoryReqs.size;

	// request a host visible memory type that can be used to copy to
	memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(_device, &memAlloc, nullptr, &stagingBuffer.memory));
	VK_CHECK(vkBindBufferMemory(_device, stagingBuffer.handle, stagingBuffer.memory, 0));

	// map the data buffer and copy vertices and indices to it, so we can use a single buffer as the source for both index and vert buffers
	uint8_t* data{ nullptr };
	VK_CHECK(vkMapMemory(_device, stagingBuffer.memory, 0, memAlloc.allocationSize, 0, (void**)&data));
	memcpy(data, vertices.data(), vertexBufferSize);
	memcpy(((char*)data) + vertexBufferSize, indices.data(), indexSize);

	// create a device local buffer to which vertex data will be copied and which will be used for rendering
	VkBufferCreateInfo vertexBufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertexBufferCI.size = vertexBufferSize;
	VK_CHECK(vkCreateBuffer(_device, &vertexBufferCI, nullptr, &vertexBuffer.handle));
	vkGetBufferMemoryRequirements(_device, vertexBuffer.handle, &memoryReqs);
	memAlloc.allocationSize = memoryReqs.size;
	memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(_device, &memAlloc, nullptr, &vertexBuffer.memory));
	VK_CHECK(vkBindBufferMemory(_device, vertexBuffer.handle, vertexBuffer.memory, 0));

	// the same for indices
	VkBufferCreateInfo indexBufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	indexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	indexBufferCI.size = indexSize;
	VK_CHECK(vkCreateBuffer(_device, &indexBufferCI, nullptr, &indexBuffer.handle));
	vkGetBufferMemoryRequirements(_device, indexBuffer.handle, &memoryReqs);
	memAlloc.allocationSize = memoryReqs.size;
	memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(_device, &memAlloc, nullptr, &indexBuffer.memory));
	VK_CHECK(vkBindBufferMemory(_device, indexBuffer.handle, indexBuffer.memory, 0));

	// need to submit copies to a queue, so create cmd buffer for em
	VkCommandBuffer copyCmd;

	VkCommandBufferAllocateInfo cmdAllocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdAllocInfo.commandPool = _immediate._immediateCommandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &copyCmd));

	VkCommandBufferBeginInfo cmdBufBeginInfo = vkinit::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(copyCmd, &cmdBufBeginInfo));

	// copying vertex and index buffers to the device
	VkBufferCopy copyRegion{};
	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, vertexBuffer.handle, 1, &copyRegion);
	copyRegion.size = indexSize;

	// offset
	copyRegion.srcOffset = vertexBufferSize;
	vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, indexBuffer.handle, 1, &copyRegion);
	VK_CHECK(vkEndCommandBuffer(copyCmd));

	// submit cmd buffer to queue
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VkFence fence;
	VK_CHECK(vkCreateFence(_device, &fenceCI, nullptr, &fence));
	// submit copies to the queue
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, fence));

	// wait for the fence to signal that command buffer has finished execution
	VK_CHECK(vkWaitForFences(_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
	vkDestroyFence(_device, fence, nullptr);
	vkFreeCommandBuffers(_device, _immediate._immediateCommandPool, 1, &copyCmd);

	// safely delete the staging buffer
	vkDestroyBuffer(_device, stagingBuffer.handle, nullptr);
	vkFreeMemory(_device, stagingBuffer.memory, nullptr);

}

void VulkanEngine::CreateUniformBuffers()
{
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = sizeof(ShaderData);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	// create the buffers
	for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		VK_CHECK(vkCreateBuffer(_device, &bufferInfo, nullptr, &_uniformBuffers[i].handle));

		// get memory requirements 
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(_device, _uniformBuffers[i].handle, &memReqs);
		VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = memReqs.size;

		// buffer would be coherent, so dont need to flush every update
		allocInfo.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// allocate memory for the uni buff
		VK_CHECK(vkAllocateMemory(_device, &allocInfo, nullptr, &_uniformBuffers[i].memory));

		// bind memory to buff
		VK_CHECK(vkBindBufferMemory(_device,
			_uniformBuffers[i].handle, _uniformBuffers[i].memory, 0));

		// map buffer once, so can update it without having to map again
		VK_CHECK(vkMapMemory(_device, _uniformBuffers[i].memory,
			0, sizeof(ShaderData), 0, (void**)&_uniformBuffers[i].mapped));
	}
}

void VulkanEngine::CreateDescriptors()
{
	VkDescriptorPoolSize descriptorTypeCounts[1]{};

	// we use only one desc(uniform buffer)
	descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;

	VkDescriptorPoolCreateInfo descriptorPoolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCI.poolSizeCount = 1;
	descriptorPoolCI.pPoolSizes = descriptorTypeCounts;

	// set max number of descriptor sets that can be requested from this pool
	descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;
	VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCI, nullptr, &_descriptorPool));

	// define interface between application and shader
	// connect different shader stages to descriptors for binding uniform buffers
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
	};
	descriptorLayoutCI.bindingCount = 1;
	descriptorLayoutCI.pBindings = &layoutBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &_drawImageDescriptorLayout));


	for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_drawImageDescriptorLayout;
		VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &_uniformBuffers[i].descriptorSet));

		// update descriptor set determining the shader binding points
		// for every binding point used in a shader there needs to be one
		// desc matching that binding point
		VkWriteDescriptorSet writeDescSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _uniformBuffers[i].handle;
		bufferInfo.range = sizeof(ShaderData);

		// binding 0 - ubo
		writeDescSet.dstSet = _uniformBuffers[i].descriptorSet;
		writeDescSet.descriptorCount = 1;
		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescSet.pBufferInfo = &bufferInfo;
		writeDescSet.dstBinding = 0;
		vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, VK_NULL_HANDLE);
	}

}

// request device memory type that support all the property flags we need
uint32_t VulkanEngine::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i)
	{
		if ((typeBits & 1) == 1)
		{
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		typeBits >>= 1;
	}
	throw "Unable to find a suitable memory type";
}
