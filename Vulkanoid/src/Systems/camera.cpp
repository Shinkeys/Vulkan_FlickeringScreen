#include "../../headers/camera.h"

void Camera::Update()
{
	static float lastFrame = 0.0f;
	const float currentFrame = glfwGetTime();
	const float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	if (Window::GetKeysStatus().left == true)
	{
		_position -= glm::normalize(_right) * deltaTime;
	}
	if (Window::GetKeysStatus().right == true)
	{
		_position += glm::normalize(_right) * deltaTime;
	}
	if (Window::GetKeysStatus().front == true)
	{
		_position += _direction * deltaTime;
	}
	if (Window::GetKeysStatus().back == true)
	{
		_position -= _direction * deltaTime;
	}
}