#pragma once
// for vulkan transit coordinates from [-1,1] to [0,1]
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <../glm/glm/glm.hpp>
#include "../vendor/glm/glm/gtc/matrix_transform.hpp"

#include "window.h"

class Camera
{
private:
	glm::vec3 _position{ 0.0f, 1.0f, 3.0f };
	const glm::vec3 _up{ 0.0f, 1.0f, 0.0f };
	const glm::vec3 _direction{ 0.0f, 0.0f, 1.0f };
	const glm::vec3 _right{ glm::normalize(glm::cross(_up, _direction)) };
public:
	glm::vec3 GetPosition() { return _position; }
	glm::mat4 GetLookToMatrix() { return glm::lookAt(_position, glm::vec3(0.0f), _up); }
	void Update();	
};