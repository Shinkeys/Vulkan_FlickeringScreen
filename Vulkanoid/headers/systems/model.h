#pragma once

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>

#include "VertexTypes.h"

class ModelLoader
{
public:
	ModelLoader()
	{
		FillVectorOfPaths();
		SetupModelLoader();
	}
	const std::vector<Mesh>& GetMeshData() const { return _mesh; }
	void SetupModelLoader();
	const std::vector<std::filesystem::path>& GetModelsPaths() const { return _pathToModels; }
private:
	void FillVectorOfPaths();
	std::vector<std::filesystem::path> _pathToModels;
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
	Texture LoadMaterials();
	std::vector<aiTextureType> _textureTypes;
	void LoadModel(std::filesystem::path& pathToModel);
	void ProcessNode(aiNode* node, const aiScene* scene);
	std::vector<Mesh> _mesh;


};