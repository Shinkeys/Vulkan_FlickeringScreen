#include "../headers/vk_swapchain.h"
#include "../headers/vk_initializers.h"

#if defined (_WIN32)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__linux__)
#include <vulkan/vulkan_xcb.h>
//#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
//#include <vulkan/vulkan_wayland.h>
#endif

#include <vector>

VkSurfaceKHR   vkswapchain::CreateWindowSurface(VkInstance instance, GLFWwindow* window)
{
#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(window);
	createInfo.hinstance = GetModuleHandle(0);

	VkSurfaceKHR surface;
	VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface));

	return surface;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	VkWaylandSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = glfwGetWaylandDisplay();
	createInfo.surface = glfwGetWaylandWindow(window);


	VkSurfaceKHR surface;
	VK_CHECK(vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface));

	return surface;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	VkXlibSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
	createInfo.dpy = glfwGetX11Display();
	createInfo.window = glfwGetX11Window(window);

	VkSurfaceKHR surface = 0;
	VK_CHECK(vkCreateXlibSurfaceKHR(instance, &createInfo, 0, &surface));
	return surface;
#else // fallback proceeding to glfw
	VkSurfaceKHR surface;
	VK_CHECK(glfwCreateWindowSurface(instance, window, 0, &surface));

	return surface;
#endif
}
VulkanSwapchain vkswapchain::CreateSwapchain(VkDevice device, VkSurfaceKHR surface, 
	VkSurfaceCapabilitiesKHR surfaceCaps, VkPresentModeKHR presentMode,  
	VkFormat format, VkExtent2D extent, uint32_t familyIndex)
{
	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = surface;
								  // thanks windows macro
	uint32_t imageCount = ((std::max)(2u, surfaceCaps.minImageCount + 1));
	swapchainInfo.minImageCount = surfaceCaps.maxImageCount > (imageCount + 1) ? 
		(imageCount + 1) : surfaceCaps.maxImageCount;
	swapchainInfo.imageFormat = format;
	swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // to check
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 1;
	swapchainInfo.pQueueFamilyIndices = &familyIndex;
	swapchainInfo.preTransform = surfaceCaps.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchainInfo.clipped = VK_TRUE; // clipping pixels which are not visible to u right now
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE; // to check
	swapchainInfo.pNext = VK_NULL_HANDLE;

	VulkanSwapchain swapchain;

	VK_CHECK(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain.swapchain));

	// IMAGES
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, nullptr));
	swapchain.swapchainImages.resize(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, swapchain.swapchainImages.data()));

	// IMAGE VIEWS
	swapchain.swapchainImageView.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo createInfo =
			vkinit::ImageViewCreateInfo(format, swapchain.swapchainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &swapchain.swapchainImageView[i]));
	}

	return swapchain;
}
VkPresentModeKHR vkswapchain::GetSwapchainPresentMode(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	uint32_t modeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &modeCount, nullptr));

	assert((modeCount > 0 && "Vulkan swapchain creation error. Unable to get phys device surface present modes!\n"));
	std::vector<VkPresentModeKHR> presentModes(modeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &modeCount, presentModes.data()));

	for (const auto& mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}
VkFormat vkswapchain::GetSwapchainSurfaceFormat(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	uint32_t formatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr));

	assert((formatCount > 0 && "Vulkan swapchain creation error. Unable to get phys device surface formats!\n"));
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, surfaceFormats.data()));

	if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return VK_FORMAT_R8G8B8A8_UNORM;
	}

	for (const auto& format : surfaceFormats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
		{
			return format.format;
		}
	}

	return surfaceFormats[0].format;

}
VkExtent2D vkswapchain::GetSwapchainExtent(VkPhysicalDevice physDevice, VkSurfaceKHR surface, GLFWwindow* window)
{
	VkSurfaceCapabilitiesKHR capabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &capabilities));

	int width{0}, height{0};
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D extent;
	extent.width = width;
	extent.height = height;

	return extent;
}
			 