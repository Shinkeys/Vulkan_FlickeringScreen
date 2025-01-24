#pragma once


#include <../glm/glm/glm.hpp>
#include <assimp/types.h>

#include <filesystem>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Texture
{
	uint32_t id;
	std::filesystem::path path;
};


struct Mesh
{
	std::vector<Vertex> vertex;
	std::vector<uint32_t> indices;
	Texture textures;
};