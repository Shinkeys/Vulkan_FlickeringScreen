#include "../headers/vk_engine.h"

#include <GLFW/glfw3.h>

#include "../headers/types/vk_types.h"
#include "../headers/vk_initializers.h"
#include "../headers/vk_pipelines.h"
#include "../headers/vk_device.h"
#include "../headers/vk_swapchain.h"

#include <iostream>
#include <array>
#include <thread>
#include <chrono>
constexpr bool bUseValidationLayers{ true };




void VulkanEngine::init()
{
	_windowManager->InitializeGLFW();

	InitVulkan();


	InitSwapchain();
	InitCommands();

	InitSyncStructures();

	PassVulkanStructures();
	SetupExternalVulkanStructures();

	SetupDescriptor();

	CreatePipeline();

	_imguiHelper.InitImgui(_windowManager->GetWindowInstance(), _swapchainImageFormat);

	// Everything went fine
	_isInitialized = true;
}

void VulkanEngine::PassVulkanStructures()
{
	_mesh.SetStructures(_device, _physDevice, _immediate._immediateCommandPool, _graphicsQueue.family);
}

void VulkanEngine::InitVulkan()
{

	// grab instance
	_instance = vkdevice::CreateInstance();
	_debugMessenger = vkdebug::CreateDebugCallback(_instance);

	_windowManager->CreateWindowSurface(_instance, &_surface);
	

	uint32_t deviceCount{ 0 };
	VK_CHECK(vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr));
	assert((deviceCount > 0, "Failed to find suitable GPUs! Error.\n"));
	
	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data()));
	// get the vkdevice handle used in the rest of vulkan application
	_physDevice = vkdevice::ChoosePhysicalDevice(devices.data(), deviceCount);
	_device = vkdevice::CreateDevice(_physDevice, _graphicsQueue.index);


	_graphicsQueue.index = vktool::GetGraphicsFamilyIndex(_physDevice);
	vkGetDeviceQueue(_device, _graphicsQueue.index, 0, &_graphicsQueue.family);

	// depth
	vktool::GetSupportedDepthFormat(_physDevice, &_depthFormat);
	SetupDepthStencil();
}
void VulkanEngine::InitSwapchain()
{
	CreateSwapchain(_windowManager->GetWindowWidth(), _windowManager->GetWindowHeight());
}
void VulkanEngine::InitCommands()
{
	// creating a command pool for commands submitted to the graphics queue
	VkCommandPoolCreateInfo	commandPoolInfo{ vkinit::CommandPoolCreateInfo(_graphicsQueue.index,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) };

	for (int i = 0; i < g_MAX_CONCURRENT_FRAMES; ++i)
	{
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		// allocate the default command buffer that will be used for render
		VkCommandBufferAllocateInfo cmdAllocInfo{ vkinit::CommandBufferAllocateInfo(_frames[i]._commandPool, 1) };


		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

		_resources.PushFunction([=]
			{
				vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
			});
	}

	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immediate._immediateCommandPool));

	_resources.PushFunction([=]
		{
			vkDestroyCommandPool(_device, _immediate._immediateCommandPool, nullptr);
		});

	// allocating buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo{ vkinit::CommandBufferAllocateInfo(_immediate._immediateCommandPool, 1) };

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immediate._immediateCommandBuffer));

}
void VulkanEngine::InitSyncStructures()
{
	// 1 fence to control when gpu finished rendering the frame,
	// and 2 semaphores to synchronize rendering with swapchain

	VkFenceCreateInfo fenceCreateInfo{ vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT) };
	VkSemaphoreCreateInfo semaphoreCreateInfo{ vkinit::SemaphoreCreateInfo() };
	/////////////////////////////////////////////////////////////////
	//////////// CHECK SEMAPHORE CODE LATER /////////////////////////
	////////////////////////////////////////////////////////////////

	for (int i = 0; i < g_MAX_CONCURRENT_FRAMES; ++i)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

		_resources.PushFunction([=]
			{
				vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
				vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
				vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			});
	}

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immediate._immediateFence));

	_resources.PushFunction([=]
		{
			vkDestroyFence(_device, _immediate._immediateFence, nullptr);
		});

	_uniformBuffers = vkutil::CreateUniformBuffers(_device, _physDevice);

}
// swapchain // 
void VulkanEngine::CreateSwapchain(uint32_t width, uint32_t height)
{
	_surface = vkswapchain::CreateWindowSurface(_instance, _windowManager->GetWindowInstance());

	VkBool32 presentSupported{ false };
	VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_physDevice, _graphicsQueue.index, _surface, &presentSupported));
	assert((presentSupported, "Vulkan error. Unable to find GPU with surface support!"));

	VkSurfaceCapabilitiesKHR capabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physDevice, _surface, &capabilities));

	_swapchainImageFormat = vkswapchain::GetSwapchainSurfaceFormat(_physDevice, _surface);

	VkPresentModeKHR presentMode = vkswapchain::GetSwapchainPresentMode(_physDevice, _surface);

	
	// VK_PRESENT_MODE_FIFO_KHR - makes a queue of images and swapchain takes images from front...
	// if queue is full then program should wait. Usually must have
	// VK_PRESENT_MODE_MAILBOX_KHR - makes a queue of images and swawpchain takes images from front...
	// if queue is full then the program replace images that are already queued...
	// used to render frames as FAST as POSSIBLE
	// although mailbox maybe not available for android, so would query for supported type.
	_swapchainExtent = vkswapchain::GetSwapchainExtent(_physDevice, _surface, _windowManager->GetWindowInstance());

	VulkanSwapchain swapchainData = vkswapchain::CreateSwapchain(_device, _surface, capabilities, presentMode,
		_swapchainImageFormat, _swapchainExtent, _graphicsQueue.index);

	_swapchain = swapchainData.swapchain;

	_swapchainImages = swapchainData.swapchainImages;
	_swapchainImagesView = swapchainData.swapchainImageView;
}

void VulkanEngine::UpdateSwapchain()
{
	if (_windowManager->HandleResize() & 
		VulkanoidOperations::VULKANOID_RESIZE_SWAPCHAIN)
	{
		VK_CHECK(vkDeviceWaitIdle(_device));
		DestroySwapchain(); // destroying old swapchain
		CreateSwapchain(_windowManager->GetWindowWidth(), _windowManager->GetWindowHeight());
		DestroyDepthStencil();
		SetupDepthStencil();
	}
}

void VulkanEngine::DestroySwapchain()
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
	/*vkDestroyDescriptorPool(_device, globalDescriptor._pool, nullptr);*/
	for (uint32_t i = 0; i < g_MAX_CONCURRENT_FRAMES; ++i)
	{
		vkDestroyBuffer(_device, _uniformBuffers[i].handle, nullptr);
		vkFreeMemory(_device, _uniformBuffers[i].memory, nullptr);
	}

	
}


void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		vkDeviceWaitIdle(_device);


		_resources.FlushQueue();

		CleanBuffers();
		DestroySwapchain();

		_globalDescriptor.Cleanup();
		_mesh.Cleanup();


		DestroyDepthStencil();

		_imguiHelper.DestroyImgui();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);

		vkdebug::DestroyDebugCallback(_instance, _debugMessenger);

		vkDestroyInstance(_instance, nullptr);
		
		// destroying window
		glfwDestroyWindow(_windowManager->GetWindowInstance());
		glfwTerminate();
	}
}

void VulkanEngine::draw()
{
	_camera.Update();
	UpdateSwapchain();
	// waiting until gpu has finished rendering last frame. 1 sec between
	VK_CHECK(vkWaitForFences(_device, 1, &GetCurrentFrame()._renderFence, VK_TRUE, UINT64_MAX)); 

	uint32_t imageIndex{};

	VK_CHECK(vkResetFences(_device, 1, &GetCurrentFrame()._renderFence));

	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, 
		GetCurrentFrame()._swapchainSemaphore, nullptr, &imageIndex));

	// update uniform buffer for the next frame
	ShaderData shaderData{};
	glm::mat4 projection = glm::perspective(90.0f, static_cast<float>(_windowManager->GetWindowWidth()) /
		static_cast<float>(_windowManager->GetWindowHeight()), 0.1f, 100.0f);
	glm::mat4 view = _camera.GetLookToMatrix();
	glm::mat4 model = glm::mat4(1.0f);
	/*model = glm::scale(model, glm::vec3(0.1f));*/

	shaderData.projMatrix = projection;
	shaderData.viewMatrix = view;
	shaderData.modelMatrix = model;

	// copy matrices to the current frame UBO
	memcpy(_uniformBuffers[g_frameNumber].mapped, &shaderData, sizeof(ShaderData));


	VkCommandBuffer cmd = GetCurrentFrame()._mainCommandBuffer;


	// now we ensure that the commands finished executing, we can safely reset command buffer
	VK_CHECK(vkResetCommandBuffer(cmd, 0));


	// begin the command buffer recording
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::CommandBufferCreateInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));


	// transition img and swapchain img into correct  transfer layouts
	vkutil::TransitionImage(cmd, _swapchainImages[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	// transition image for depth
	vkutil::TransitionImage(cmd, _depthStencil.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
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
	renderingInfo.renderArea = { 0, 0, _windowManager->GetWindowWidth(), _windowManager->GetWindowHeight() };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;
	renderingInfo.pStencilAttachment = &depthStencilAttachment;


	
	// Start a dynamic rendering section
	vkCmdBeginRendering(cmd, &renderingInfo);
	// Update dynamic viewport state
	VkViewport viewport{ 0.0f, 0.0f, (float)_windowManager->GetWindowWidth(),
		(float)_windowManager->GetWindowHeight(), 0.0f, 1.0f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	// Update dynamic scissor state
	VkRect2D scissor{ 0, 0, _windowManager->GetWindowWidth(), _windowManager->GetWindowHeight() };
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	// Bind descriptor set for the current frame's uniform buffer, so the shader uses the data from that buffer for this draw
	//  &_uniformBuffers[_frameNumber].descriptorSet,
	VkDescriptorSet set = _globalDescriptor.GetDescriptorSet();
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1,  &set, 0, nullptr);
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	// Bind triangle vertex buffer (contains position and colors)
	_mesh.DrawMeshes(cmd, _pipelineLayout);
	// Finish the current dynamic rendering section
	vkCmdEndRendering(cmd);

	_imguiHelper.DrawImgui(cmd, _swapchainImagesView[imageIndex], _swapchainExtent);

	// setting swapchain image layout to present
	vkutil::TransitionImage(cmd, _swapchainImages[imageIndex],
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	// finalize cmd buffer
	VK_CHECK(vkEndCommandBuffer(cmd));


	//prepare the submission to the queue. 
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = vkinit::CommandBufferSubmitInfo(cmd);

	VkSemaphoreSubmitInfo waitInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame()._swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame()._renderSemaphore);

	VkSubmitInfo2 submit = vkinit::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue.family, 1, &submit, GetCurrentFrame()._renderFence));

	//prepare present
	// this will put the image we just rendered to into the visible window.
	// we want to wait on the _renderSemaphore for that, 
	// as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &GetCurrentFrame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &imageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue.family, &presentInfo));

	//increase the number of frames drawn
	g_frameNumber = (g_frameNumber + 1) % g_MAX_CONCURRENT_FRAMES;
		
}

void VulkanEngine::run()
{
	bool bQuit = false;
	bool minimizedWindow = false;

	// Main loop
	while (!bQuit && !glfwWindowShouldClose(_windowManager->GetWindowInstance()))
	{
		if (!glfwGetWindowAttrib(_windowManager->GetWindowInstance(), GLFW_MAXIMIZED))
		{
			minimizedWindow = true;
		}
		// Poll for and process events
		glfwPollEvents();

		_imguiHelper.Update();
		// Draw function placeholder
		draw();

		// Check if the window should close (like pressing X button)
		if (glfwWindowShouldClose(_windowManager->GetWindowInstance())) {
			bQuit = true;
		}
	}
}


// PIPELINES


void VulkanEngine::CreatePipeline()
{
	VkDescriptorSetLayout descLayout = _globalDescriptor.GetDescriptorSetLayout();

	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pSetLayouts = &descLayout;
	computeLayout.setLayoutCount = 1;
	// push consts
	if (_mesh.GetOperations() & VulkanoidOperations::VULKANOID_USE_CONSTANTS)
	{
		VkPushConstantRange constRange{};
		constRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		constRange.offset = 0;
		constRange.size = sizeof(PushConstant);
		computeLayout.pPushConstantRanges = &constRange;
		computeLayout.pushConstantRangeCount = 1;
	}

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
	std::array<VkVertexInputAttributeDescription, 3>  vertexInputAttributs{};
	// layout (location = 0) in vec3 inPos;
	// layout (location = 1) in vec3 inColor;

	// position
	vertexInputAttributs[0].binding = vertexInputBinding.binding;
	vertexInputAttributs[0].location = 0;
	vertexInputAttributs[1].binding = vertexInputBinding.binding;
	vertexInputAttributs[1].location = 1;
	vertexInputAttributs[2].binding = vertexInputBinding.binding;
	vertexInputAttributs[2].location = 2;

	// pos attribute is 3 32 bit signed float
	vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertexInputAttributs[0].offset = offsetof(Vertex, position);
	vertexInputAttributs[1].offset = offsetof(Vertex, normal);
	vertexInputAttributs[2].offset = offsetof(Vertex, texCoord);


	// vertex input state for pipeline creation
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputStateCI.vertexBindingDescriptionCount = 1;
	vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputStateCI.vertexAttributeDescriptionCount = 3;
	vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributs.data();
	

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


	_resources.PushFunction([=]
		{
			vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
			vkDestroyPipeline(_device, _pipeline, nullptr);
		});

}

void VulkanEngine::SetupDepthStencil()
{
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(_physDevice, &deviceMemoryProperties);

	// Create an optimal tiled image used as the depth stencil attachment
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = _depthFormat;
	imageCI.extent = { _windowManager->GetWindowWidth(), _windowManager->GetWindowHeight(), 1 };
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
	memAlloc.memoryTypeIndex = vktool::GetMemoryTypeIndex(deviceMemoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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

void VulkanEngine::DestroyDepthStencil()
{
	vkDestroyImage(_device, _depthStencil.image, nullptr);
	vkDestroyImageView(_device, _depthStencil.view, nullptr);
	vkFreeMemory(_device, _depthStencil.memory, nullptr);
}
	

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(_device, 1, &_immediate._immediateFence));
	VK_CHECK(vkResetCommandBuffer(_immediate._immediateCommandBuffer, 0));

	VkCommandBuffer cmd = _immediate._immediateCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::CommandBufferCreateInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdInfo = vkinit::CommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = vkinit::SubmitInfo(&cmdInfo, nullptr, nullptr);

	// submit command buffer to the queue. render fence will be blocked until the graphics commands finish exec
	VK_CHECK(vkQueueSubmit2(_graphicsQueue.family, 1, &submit, _immediate._immediateFence));

	VK_CHECK(vkWaitForFences(_device, 1, &_immediate._immediateFence, true, DEFAULT_FENCE_TIMEOUT));

}


void VulkanEngine::SetupExternalVulkanStructures()
{
	_globalDescriptor.SetDevice(_device);
	_mesh.CreateBuffers();
	_imguiHelper.SetStructures(_device, _instance, _physDevice, _graphicsQueue.family);
}

void VulkanEngine::SetupDescriptor()
{
	_globalDescriptor.DescriptorBasicSetup(_mesh.GetModels().textures.size() + 1);
	_globalDescriptor.UpdateUBOBindings(_uniformBuffers, _globalDescriptor.GetDescriptorSet());
	_globalDescriptor.UpdateBindlessBindings(_globalDescriptor.GetDescriptorSet(),
		_mesh.GetModels().textures, _mesh.GetSampler());
}


