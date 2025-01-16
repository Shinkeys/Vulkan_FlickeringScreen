#include "../../headers/systems/model.h"

void ModelLoader::SetupModelLoader()
{
	for (uint32_t i = 0; i < _pathToModels.size(); ++i)
	{
		LoadModel(_pathToModels[i]);
	}
}
void ModelLoader::FillVectorOfPaths()
{
	std::filesystem::path model1 = "objects/models/oxygen.fbx";
	_pathToModels.push_back(model1);
}

void ModelLoader::LoadModel(std::filesystem::path& pathToModel)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(pathToModel.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "Assimp: unable to load your model\n" << importer.GetErrorString() << "\n";
		return;
	}

	ProcessNode(scene->mRootNode, scene);
}

Mesh ModelLoader::ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
{
	Mesh mesh;

	mesh.vertex.reserve(aiMesh->mNumVertices);
	for (uint32_t i = 0; i < aiMesh->mNumVertices; ++i)
	{
		Vertex vertices;
		vertices.position.x = aiMesh->mVertices[i].x;
		vertices.position.y = aiMesh->mVertices[i].y;
		vertices.position.z = aiMesh->mVertices[i].z;
		
		if (aiMesh->mTextureCoords[0])
		{
			vertices.uv.x = aiMesh->mTextureCoords[0][i].x;
			vertices.uv.y = aiMesh->mTextureCoords[0][i].y;
		}
		else { vertices.uv = glm::vec2(0.0f); }

		if (aiMesh->HasNormals())
		{
			vertices.normal.x = aiMesh->mNormals[0].x;
			vertices.normal.y = aiMesh->mNormals[0].y;
			vertices.normal.z = aiMesh->mNormals[0].z;
		}
		// filling data at the end
		mesh.vertex.push_back(vertices);
	}
	
	mesh.indices.reserve(aiMesh->mNumFaces);
	for (uint32_t i = 0; i < aiMesh->mNumFaces; ++i)
	{
		aiFace face = aiMesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; ++j)
		{
			mesh.indices.push_back(face.mIndices[j]);
		}
	}

	
	/*if (aiMesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
	}*/

	return Mesh(mesh);
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		_mesh.push_back(ProcessMesh(mesh, scene));
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}