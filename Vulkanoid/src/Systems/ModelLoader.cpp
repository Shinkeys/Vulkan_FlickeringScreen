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
	std::filesystem::path model1 = "objects/models/blacksmith/scene.gltf";
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

	std::vector<Vertex> sortedVertices;
	sortedVertices.reserve(aiMesh->mNumVertices);
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
			vertices.normal.x = aiMesh->mNormals[i].x;
			vertices.normal.y = aiMesh->mNormals[i].y;
			vertices.normal.z = aiMesh->mNormals[i].z;
		}
		sortedVertices.push_back(vertices);
	}
	
	mesh.indices.reserve(aiMesh->mNumFaces);
	static uint32_t offset = 0;
	for (uint32_t i = 0; i < aiMesh->mNumFaces; ++i)
	{
		aiFace face = aiMesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; ++j)
		{
			mesh.indices.push_back(offset);
			mesh.vertex.push_back(sortedVertices[face.mIndices[j]]);

			++offset;
		}
	}

	
	if (aiMesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
		std::array<aiTextureType, 4> textureTypes
		{
			aiTextureType_DIFFUSE,
			aiTextureType_SPECULAR,
			aiTextureType_EMISSIVE,
			aiTextureType_HEIGHT
		};

		mesh.textures = ProcessMaterial(material, textureTypes);
	}

	return mesh;
}

Texture ModelLoader::ProcessMaterial(aiMaterial* material, std::array<aiTextureType, 4> textureTypes)
{
	Texture textures{};
	// need it when loading a lot of models, for example 10 models
	// id should be unique for every texture
	static uint32_t offset = 0;
	for (uint32_t i = 0; i < textureTypes.size(); ++i)
	{
		const uint32_t currentTextureCount = material->GetTextureCount(textureTypes[i]);
		for (uint32_t j = 0; j < currentTextureCount; ++j)
		{
			aiString str;
			// to replace index later
			material->GetTexture(textureTypes[i], j, &str);
			switch (textureTypes[i])
			{
			case aiTextureType_DIFFUSE: textures.diffuse_id = j + offset; textures.diffuse_path = str; break;
			case aiTextureType_SPECULAR: textures.specular_id = j + offset; textures.specular_path = str; break;
			case aiTextureType_EMISSIVE: textures.emissive_id = j + offset; textures.emissive_path = str; break;
			case aiTextureType_HEIGHT: textures.normal_id = j + offset; textures.normal_path = str; break;
			}

		}
		offset += static_cast<uint32_t>(currentTextureCount);
	}

	return textures;
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