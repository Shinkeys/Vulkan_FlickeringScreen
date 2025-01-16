#pragma once
// for vulkan transit coordinates from [-1,1] to [0,1]
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <../glm/glm/glm.hpp>
#include "../../vendor/glm/glm/gtc/matrix_transform.hpp"

#include "../window.h"

class Camera
{
private:
	// y is reverse in vulkan
	glm::vec3 _position{ 0.0f, -1.0f, 3.0f };
	glm::vec3 _direction{ 0.0f, 0.0f, 1.0f };
	glm::vec3 _up{ 0.0f, -1.0f, 0.0f };
	glm::vec3 _right{ glm::normalize(glm::cross(_up, _direction)) };
	void CalculateDirection();
	void CalculateKeyboard();
	float yaw = -90.0f;
	float pitch = 0.0f;
	
	Window* _window;
public:
	void Update();
	Camera(Window* window);
	glm::vec3 GetPosition() { return _position; }
	glm::mat4 GetLookToMatrix() { return glm::lookAt(_position, _position + _direction, _up); }
};