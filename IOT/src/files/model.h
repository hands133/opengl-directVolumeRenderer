#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include <platform/openGL/vrOpenGLShader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

class Model {
public:
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;

	aiAABB BBX;

	Model(bool gamma = false) : gammaCorrection(gamma)
	{
		BBX.mMin = { FLT_MAX, FLT_MAX, FLT_MAX };
		BBX.mMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	}
	~Model() {}

	void LoadModel(const std::string& path)
	{
		this->loadModel(path);
	}

	void Draw(tinyvr::vrOpenGLShader& shader)
	{
		for (auto& mesh : meshes)	mesh.Draw(shader);
	}

private:
	void loadModel(const std::string& path)
	{
		Assimp::Importer importer;
		auto* scene = importer.ReadFile(path,
			aiProcess_Triangulate | aiProcess_FlipUVs | 
			aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes |
			aiProcess_GenNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !(scene->mRootNode))
		{
			std::cout << "ERROR:ASSIMP:: " << importer.GetErrorString() << '\n';
			return;
		}

		directory = path.substr(0, path.find_last_of('/'));

		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		std::cout << "node->numMeshes = " << node->mNumMeshes << '\n'
			<< "node->numChildren = " << node->mNumChildren << '\n';

		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			auto bbx = mesh->mAABB;
			if (bbx.mMin.x < BBX.mMin.x)	BBX.mMin.x = bbx.mMin.x;
			if (bbx.mMin.y < BBX.mMin.y)	BBX.mMin.y = bbx.mMin.y;
			if (bbx.mMin.z < BBX.mMin.z)	BBX.mMin.z = bbx.mMin.z;
			if (bbx.mMax.x > BBX.mMax.x)	BBX.mMax.x = bbx.mMax.x;
			if (bbx.mMax.y > BBX.mMax.y)	BBX.mMax.y = bbx.mMax.y;
			if (bbx.mMax.z > BBX.mMax.z)	BBX.mMax.z = bbx.mMax.z;

			meshes.emplace_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
			processNode(node->mChildren[i], scene);
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		std::cout << "mesh->numVertices = " << mesh->mNumVertices << '\n';

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex;
			glm::vec3 vector;

			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			// normals
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;

			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.emplace_back(vertex);
		}

		std::cout << "mesh->numFaces = " << mesh->mNumFaces << '\n';
		
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.emplace_back(face.mIndices[j]);
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		return Mesh(vertices, indices, textures);
	}
};