#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "camera.h"

#include <vulkan/vulkan.h>
struct Keys
{
	bool front{ false };
	bool back{ false };
	bool left{ false };
	bool right{ false };
};

class Window
{
public:
	void InitializeGLFW();
	void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	GLFWwindow* GetWindowInstance() const { return _window; }
	const uint32_t GetWindowWidth() const { return _width; }
	const uint32_t GetWindowHeight() const { return _height; }
	static const Keys& GetKeysStatus() { return _keys; }
private:
	void ProceedKeys(int key);
	void ResetKey(int key);
	static void KeyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods);

	inline static Keys _keys;
	struct GLFWwindow* _window;
	uint32_t _width{ 1920 };
	uint32_t _height{ 1080 };

};