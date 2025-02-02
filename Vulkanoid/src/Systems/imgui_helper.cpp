#include "../../headers/systems/imgui_helper.h"


#include "../../vendor/imgui/imgui.h"
#include "../../vendor/imgui/imgui_impl_vulkan.h"
#include "../../vendor/imgui/imgui_impl_glfw.h"

void ImguiHelper::InitImgui(GLFWwindow* window, VkFormat swapchainImgFormat)
{
	// creating descriptor pool for imgui
	VkDescriptorPoolSize poolSizes[] = {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 0;
	for (VkDescriptorPoolSize& poolSize : poolSizes)
	{
		poolInfo.maxSets += poolSize.descriptorCount;
	}
	poolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;
	VK_CHECK(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_imguiPool));

	// INITIALIZING IMGUI LIB
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, false);

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _physDevice;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = _imguiPool;
	init_info.MinImageCount = g_MAX_CONCURRENT_FRAMES;
	init_info.ImageCount = g_MAX_CONCURRENT_FRAMES;
	init_info.UseDynamicRendering = true;

	// dynamic rendering parameters
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImgFormat;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

}
void ImguiHelper::DrawImgui(VkCommandBuffer cmdBuffer, VkImageView targetImageView, VkExtent2D swapchainExtent)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(targetImageView,
		nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::RenderingInfo(swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmdBuffer, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

	vkCmdEndRendering(cmdBuffer);
}
void ImguiHelper::DestroyImgui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(_device, _imguiPool, nullptr);
}

void ImguiHelper::ImguiSettings()
{
	ImGui::SetNextWindowPos({ 0,0 });
}

void ImguiHelper::ImguiWindows()
{
	ImGui::SetNextWindowSize(ImVec2(350, 500));
	ImGui::SetNextWindowPos({ 0,0 });
	ImGui::Begin("Vulkanoid debug");
	ImGui::End();
}

void ImguiHelper::Update()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImguiWindows();
	ImGui::Render();
}

void ImguiHelper::SetStructures(VkDevice device, VkInstance instance, VkPhysicalDevice physDevice, VkQueue graphicsQueue)
{
	this->_device = device;
	this->_instance = instance;
	this->_physDevice = physDevice;
	this->_graphicsQueue = graphicsQueue;
}