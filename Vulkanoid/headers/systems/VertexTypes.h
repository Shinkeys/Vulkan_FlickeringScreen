#pragma once

#include <string>
#include <../glm/glm/glm.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Texture
{
	uint32_t id;
	std::string type;
};


struct Mesh
{
	std::vector<Vertex> vertex;
	std::vector<uint32_t> indices;
};