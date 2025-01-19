#pragma once

#include <string>
#include <unordered_map>
#include <../glm/glm/glm.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Texture
{
	uint32_t diffuse_id{ 0 };
	uint32_t specular_id{ 0 };
	uint32_t emissive_id{ 0 };
	uint32_t normal_id{ 0 };
	aiString diffuse_path;
	aiString specular_path;
	aiString emissive_path;
	aiString normal_path;
};


struct Mesh
{
	std::vector<Vertex> vertex;
	std::vector<uint32_t> indices;
	Texture textures;
};