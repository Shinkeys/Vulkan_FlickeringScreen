#include "../../headers/systems/camera.h"

Camera::Camera(Window* window) : _window{ window }
{
}

void Camera::Update()
{
	CalculateDirection();
	CalculateKeyboard();
}

void Camera::CalculateKeyboard()
{
	static float lastFrame = 0.0f;
	const float currentFrame = glfwGetTime();
	const float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	if (_window->GetKeysState().left)
	{
		_position += glm::normalize(_right) * deltaTime;
	}
	if (_window->GetKeysState().right)
	{
		_position -= glm::normalize(_right) * deltaTime;
	}
	// z axis is reversed in vulkan
	if (_window->GetKeysState().front)
	{
		_position -= _direction * deltaTime;
	}
	if (_window->GetKeysState().back)
	{
		_position += _direction * deltaTime;
	}
}

void Camera::CalculateDirection()
{
	const float sensitivity = 0.005f;

	float rotateX = _window->GetLastMouseOffset().x;
	float rotateY = _window->GetLastMouseOffset().y;

	glm::mat4 rotation = glm::mat4(1.0f);

	if (rotateX || rotateY)
	{
		float yaw = rotateX * sensitivity;
		float pitch = rotateY * sensitivity;


		rotation = glm::rotate(rotation, -yaw, _up);
		rotation = glm::rotate(rotation, -pitch, _right);
	}
	_direction = glm::normalize(glm::mat3(rotation) * _direction);
	_up = glm::normalize(glm::mat3(rotation) * _up);
	_right = glm::normalize(glm::cross(_direction, _up));

	_window->ResetMouse();
}