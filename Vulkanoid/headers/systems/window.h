#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan.h>

#include "../types/vk_types.h"

struct Keys
{
	bool front{ false };
	bool back{ false };
	bool left{ false };
	bool right{ false };
};

struct Mouse
{
	float x{ 0.0f };
	float y{ 0.0f };
};

class Window
{
public:
	void InitializeGLFW();
	void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	GLFWwindow* GetWindowInstance() const { return _window; }
	const uint32_t GetWindowWidth() const { return _width; }
	const uint32_t GetWindowHeight() const { return _height; }
	const Mouse GetLastMouseOffset() const { return _mouse; }
	const Keys& GetKeysState() const { return _keys; }
	VulkanoidOperations HandleResize();
	void ResetMouse();
private:
	Keys _keys;
	Mouse _mouse;
	void ProceedKeys(int key);
	void ResetKey(int key);
	static void KeyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CursorCallbackGLFW(GLFWwindow* window, double xPos, double yPos);
	static void FramebufferSizeCallbackGLFW(GLFWwindow* window, int width, int height);
	void ProceedMouseMovement(double xPos, double yPos);
	struct GLFWwindow* _window;
	uint32_t _width{ 1920 };
	uint32_t _height{ 1080 };

	bool _resize = false;
	// initial position of camera(center of scr)
	float _lastMouseWidth = _width / 2;
	float _lastMouseHeight = _height / 2;

};