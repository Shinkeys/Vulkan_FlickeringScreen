#include "../headers/vk_device.h"
#include "../headers/vk_tools.h"
#include <GLFW/glfw3.h>
#include <vector>

#if defined (_WIN32)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__linux__)
#include <vulkan/vulkan_xcb.h>
#endif

#ifndef NDEBUG
constexpr bool KHR_VALIDATION{ true };
#else
constexpr bool KHR_VALIDATION{ false };
#endif

VkInstance vkdevice::CreateInstance()
{
	std::vector<std::string> supportedInstanceExtensions;
	uint32_t extensionCount{ 0 };
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	if (extensionCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extensionCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) == VK_SUCCESS)
		{
			for (VkExtensionProperties& property : extensions)
			{
				supportedInstanceExtensions.push_back(property.extensionName);
			}
		}
		else
		{
			std::cerr << "Vulkan instance creation error. No available instance extensions\n";
		}
	}




	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkanoid Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = g_API_VERSION;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// surface ext depending on OS
#if defined(_WIN32)
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	instanceExtensions.push_back(VK_KHR_WAYLAND_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifndef NDEBUG
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	instanceCreateInfo.enabledLayerCount = 0;

	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	if (KHR_VALIDATION)
	{
		const char* validationLayerName{ "VK_LAYER_KHRONOS_validation" };

		uint32_t instanceLayerCount;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data()));
		bool isLayerPresent{ false };
		for (const VkLayerProperties& property : instanceLayerProperties)
		{
			if (strcmp(property.layerName, validationLayerName) == 0)
			{
				isLayerPresent = true;
				break;
			}
		}

		if (isLayerPresent)
		{
			instanceCreateInfo.enabledLayerCount = 1;
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			// layers
			vkdebug::DebugCreateInfo(debugInfo);
			instanceCreateInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugInfo);
			std::cout << "Enabled validation layers\n";
		}
		else
		{
			std::cerr << "Vulkan Validation Layers are not loaded in vk_device\n";
		}
	}

	VkInstance instance;
	VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

	return instance;
}

VkPhysicalDevice vkdevice::ChoosePhysicalDevice(VkPhysicalDevice* physDevices, uint32_t physDevicesCount)
{
	VkPhysicalDevice preferred{ 0 };
	VkPhysicalDevice fallback{ 0 };

	for (uint32_t i = 0; i < physDevicesCount; ++i)
	{
		VkPhysicalDeviceProperties physProps;
		vkGetPhysicalDeviceProperties(physDevices[i], &physProps);

		if (physProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
		{
			continue;
		}

		const uint32_t familyIndex = vktool::GetGraphicsFamilyIndex(physDevices[i]);
		if (familyIndex == VK_QUEUE_FAMILY_IGNORED)
		{
			continue;
		}

		if (physProps.apiVersion < g_API_VERSION)
		{
			continue;
		}

		if (!preferred && physProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			preferred = physDevices[i];
		}

		if (!fallback)
		{
			fallback = physDevices[i];
		}
	}

	VkPhysicalDevice result = preferred ? preferred : fallback;

	if (result)
	{
		VkPhysicalDeviceProperties physProps;
		vkGetPhysicalDeviceProperties(result, &physProps);

		std::cout << "Selected GPU: " << physProps.deviceName << "\n";
	}
	else
	{
		throw std::runtime_error("Compatible GPU not found! Terminating process\n");
	}

	return result;
}

VkDevice vkdevice::CreateDevice(VkPhysicalDevice physDevice, uint32_t familyIndex)
{
	const char* swapchainExtension{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	constexpr float queuePriority{ 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = familyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	// enabling features from vk 1.3
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;
	// enabling features from vk 1.2
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;
	// bindless
	features12.descriptorBindingPartiallyBound = VK_TRUE;
	features12.runtimeDescriptorArray = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;
	features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;

	createInfo.ppEnabledExtensionNames = &swapchainExtension;
	createInfo.enabledExtensionCount = 1;

	createInfo.pNext = &features12;
	features12.pNext = &features13;

	VkDevice device;
	VK_CHECK(vkCreateDevice(physDevice, &createInfo, nullptr, &device));

	return device;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkdebug::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << "\n";
	return VK_FALSE;
}

void vkdebug::DebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info)
{
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; // callback event types
	info.pfnUserCallback = DebugCallback;
	info.pUserData = nullptr;
}


VkDebugUtilsMessengerEXT vkdebug::CreateDebugCallback(VkInstance instance)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{  };
	DebugCreateInfo(createInfo);
	// checking if extension present
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 
		reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	assert((vkCreateDebugUtilsMessengerEXT != nullptr && "Vulkan debug error. Cannot get extension to create debug utils!\nCompile in Release to work without layers!\n"));
	VkDebugUtilsMessengerEXT debugMessenger;
	VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
	
	return debugMessenger;
}

void vkdebug::DestroyDebugCallback(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = 
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT != nullptr)
	{
		vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
}
