#include "../../headers/window.h"


void Window::ProceedKeys(int key)
{
	if (key == GLFW_KEY_W)
	{
		_keys.front = true;
	}
	if (key == GLFW_KEY_S)
	{
		_keys.back = true;
	}
	if (key == GLFW_KEY_A)
	{
		_keys.left = true;
	}
	if (key == GLFW_KEY_D)
	{
		_keys.right = true;
	}
}

void Window::ProceedMouseMovement(double xPos, double yPos)
{
	_mouse.x = xPos - _lastMouseWidth;
	_mouse.y = yPos - _lastMouseHeight;

	_lastMouseWidth = xPos;
	_lastMouseHeight = yPos;
}

void Window::ResetMouse()
{
	_mouse.x = 0.0f;
	_mouse.y = 0.0f;
}

void Window::InitializeGLFW()
{
	// Initialize GLFW
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	// Specify that we want to use Vulkan with GLFW
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Create a windowed mode window and its OpenGL context
	_window = glfwCreateWindow(
		_width,
		_height,
		"Vulkan Engine",
		nullptr,
		nullptr
	);

	if (!_window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwSetWindowUserPointer(_window, this);

	glfwSetKeyCallback(_window, KeyCallbackGLFW);
	glfwSetCursorPosCallback(_window, CursorCallbackGLFW);
}

void Window::ResetKey(int key)
{
	switch (key)
	{
	case GLFW_KEY_W: _keys.front = false; break;
	case GLFW_KEY_S: _keys.back = false; break;
	case GLFW_KEY_A: _keys.left = false; break;
	case GLFW_KEY_D: _keys.right = false; break;
	}
}

void Window::CursorCallbackGLFW(GLFWwindow* window, double xPos, double yPos)
{
	Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	app->ProceedMouseMovement(xPos, yPos);
}
void Window::KeyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* app =  reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS)
	{
		app->ProceedKeys(key);
	}
	if (action == GLFW_RELEASE)
	{
		app->ResetKey(key);
	}


}
void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	////////
	glfwCreateWindowSurface(instance, _window, nullptr, surface);
}