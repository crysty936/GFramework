#include "AssimpModel3D.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "Logger/Logger.h"
#include "assimp/postprocess.h"
#include "Renderer/RenderingPrimitives.h"
#include "Renderer/RHI/Resources/RHITexture.h"
#include "Renderer/RHI/RHITypes.h"
#include "assimp/GltfMaterial.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"
#include <d3d12.h>
#include "EASTL/set.h"

static Transform aiMatrixToTransform(const aiMatrix4x4& inMatrix)
{
	aiVector3D aiScaling;
	aiVector3D aiRotation;
	aiVector3D aiPosition;
	inMatrix.Decompose(aiScaling, aiRotation, aiPosition);

	glm::vec3 scaling(aiScaling.x, aiScaling.y, aiScaling.z);
	glm::vec3 rotation(aiRotation.x, aiRotation.y, aiRotation.z);
	glm::vec3 translation(aiPosition.x, aiPosition.y, aiPosition.z);

	return Transform(translation, rotation, scaling);
}

AssimpModel3D::AssimpModel3D(const eastl::string& inPath, const eastl::string& inName, glm::vec3 inOverrideColor)
	: Model3D(inName), ModelPath{ inPath }, OverrideColor(inOverrideColor)
{
}

AssimpModel3D::~AssimpModel3D() = default;

void AssimpModel3D::LoadModelToRoot(const eastl::string inPath, TransformObjPtr inParent, ID3D12GraphicsCommandList* inCommandList)
{
	eastl::shared_ptr<MeshNode> mesh = LoadData(inCommandList);

	if (ENSURE(mesh))
	{
		inParent->AddChild(mesh);
	}
}

void AssimpModel3D::Init(ID3D12GraphicsCommandList* inCommandList)
{
	LoadModelToRoot(ModelPath, shared_from_this(), inCommandList);
}

eastl::shared_ptr<MeshNode> AssimpModel3D::LoadData(ID3D12GraphicsCommandList* inCommandList)
{
	Assimp::Importer modelImporter;

	const aiScene* scene = modelImporter.ReadFile(ModelPath.c_str(), 0);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		LOG_ERROR("Unable to load model from path %s", ModelPath.c_str());

		return nullptr;
	}

	ModelDir = ModelPath.substr(0, ModelPath.find_last_of('/'));

	ProcessMaterials(*scene, inCommandList);

	eastl::shared_ptr<MeshNode> newNode = eastl::make_shared<MeshNode>("RootNode");
	newNode->SetRelTransform(aiMatrixToTransform(scene->mRootNode->mTransformation));
	ProcessNodesRecursively(*scene->mRootNode, *scene, newNode, inCommandList);

	return newNode;
}

void AssimpModel3D::ProcessMaterials(const aiScene& inScene, ID3D12GraphicsCommandList* inCommandList)
{
	const uint32_t numMaterials = inScene.mNumMaterials;

	Materials.resize(numMaterials);

	for (uint32_t i = 0; i < numMaterials; ++i)
	{
		MeshMaterial& currMaterial = Materials[i];
		aiMaterial* currAsimpMat = inScene.mMaterials[i];

		currMaterial.AlbedoMap = LoadMaterialTexture(*currAsimpMat, aiTextureType_DIFFUSE, inCommandList);
		currMaterial.NormalMap = LoadMaterialTexture(*currAsimpMat, aiTextureType_NORMALS, inCommandList);
		currMaterial.MRMap = LoadMaterialTexture(*currAsimpMat, aiTextureType_UNKNOWN, inCommandList); //AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE
	}
}

void AssimpModel3D::ProcessNodesRecursively(const aiNode & inNode, const aiScene & inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, struct ID3D12GraphicsCommandList* inCommandList)
{
	for (uint32_t i = 0; i < inNode.mNumMeshes; ++i)
	{
		const uint32_t meshIndex = inNode.mMeshes[i];
		const aiMesh* assimpMesh = inScene.mMeshes[meshIndex];

		ProcessMesh(*assimpMesh, inScene, inCurrentNode, inCommandList);
	}

	for (uint32_t i = 0; i < inNode.mNumChildren; ++i)
	{
		const aiNode& nextAiNode = *inNode.mChildren[i];
		eastl::shared_ptr<MeshNode> newNode = eastl::make_shared<MeshNode>(nextAiNode.mName.C_Str());
		newNode->SetRelTransform(aiMatrixToTransform(nextAiNode.mTransformation));

		ProcessNodesRecursively(nextAiNode, inScene, newNode, inCommandList);
		inCurrentNode->AddChild((newNode));
	}
}

void AssimpModel3D::ProcessMesh(const aiMesh& inMesh, const aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, ID3D12GraphicsCommandList* inCommandList)
{
	VertexInputLayout inputLayout;
	// Vertex points
	inputLayout.Push<float>(3, VertexInputType::Position);
	// Normals
	inputLayout.Push<float>(3, VertexInputType::Normal);
	// Vertex Tex Coords
	inputLayout.Push<float>(2, VertexInputType::TexCoords);
	// Tangent
	inputLayout.Push<float>(3, VertexInputType::Tangent);
	// Bitangent
	inputLayout.Push<float>(3, VertexInputType::Bitangent);

	eastl::shared_ptr<D3D12IndexBuffer> indexBuffer;
	eastl::shared_ptr<D3D12VertexBuffer> vertexBuffer;

	{
		eastl::vector<Vertex> vertices;
		eastl::vector<uint32_t> indices;

		for (uint32_t i = 0; i < inMesh.mNumVertices; i++)
		{
			Vertex vert;
			const aiVector3D& aiVertex = inMesh.mVertices[i];
			const aiVector3D& aiNormal = inMesh.mNormals[i];

			vert.Position = glm::vec3(aiVertex.x, aiVertex.y, aiVertex.z);
			vert.Normal = glm::vec3(aiNormal.x, aiNormal.y, aiNormal.z);

			if (inMesh.HasTangentsAndBitangents())
			{
				const aiVector3D& aiTangent = inMesh.mTangents[i];
				vert.Tangent = glm::vec3(aiTangent.x, aiTangent.y, aiTangent.z);

				const aiVector3D& aiBitangent = inMesh.mBitangents[i];
				vert.Bitangent = glm::vec3(aiBitangent.x, aiBitangent.y, aiBitangent.z);
			}
			else
			{
				vert.Tangent = glm::vec3(1.f, 0.f, 0.f);
				vert.Bitangent = glm::vec3(0.f, 1.f, 0.f);
			}

			if (inMesh.mTextureCoords[0])
			{
				const aiVector3D& aiTexCoords = inMesh.mTextureCoords[0][i];
				vert.TexCoords = glm::vec2(aiTexCoords.x, aiTexCoords.y);
			}
			else
			{
				vert.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vert);
		}

		for (uint32_t i = 0; i < inMesh.mNumFaces; i++)
		{
			const aiFace& Face = inMesh.mFaces[i];

			for (uint32_t j = 0; j < Face.mNumIndices; j++)
			{
				indices.push_back(Face.mIndices[j]);
			}
		}

		const int32_t indicesCount = static_cast<int32_t>(indices.size());
		indexBuffer = D3D12RHI::Get()->CreateIndexBuffer(indices.data(), indicesCount);
		const int32_t verticesCount = static_cast<int32_t>(vertices.size());

		vertexBuffer = D3D12RHI::Get()->CreateVertexBuffer(inputLayout, (float*)vertices.data(), vertices.size(), indexBuffer);
	}

	eastl::shared_ptr<MeshNode> newMesh = eastl::make_shared<MeshNode>(inMesh.mName.C_Str());
	newMesh->IndexBuffer = indexBuffer;
	newMesh->VertexBuffer = vertexBuffer;
	newMesh->MatIndex = inMesh.mMaterialIndex;
	//newMesh->Textures = textures;

	inCurrentNode->AddChild(newMesh);
}

eastl::shared_ptr<D3D12Texture2D> AssimpModel3D::LoadMaterialTexture(const aiMaterial& inMat, const aiTextureType& inAssimpTexType, ID3D12GraphicsCommandList* inCommandList)
{
	aiString Str;
	inMat.GetTexture(inAssimpTexType, 0, &Str);
	eastl::shared_ptr<D3D12Texture2D> newTex = nullptr;

	if (Str.length != 0 && !IsTextureLoaded(Str.C_Str(), newTex))
	{
		const eastl::string path = ModelDir + eastl::string("/") + eastl::string(Str.C_Str());

		const bool srgb = inAssimpTexType == aiTextureType_DIFFUSE;
		newTex = D3D12RHI::Get()->CreateAndLoadTexture2D(path, srgb, true, inCommandList);
		newTex->SourcePath = eastl::string(Str.C_Str());
		LoadedTextures.push_back(newTex);
	}

	return newTex;
}

bool AssimpModel3D::IsTextureLoaded(const eastl::string& inTexPath, OUT eastl::shared_ptr<D3D12Texture2D>& outTex)
{
	for (const eastl::shared_ptr<D3D12Texture2D>& loadedTexture : LoadedTextures)
	{
		if (loadedTexture->SourcePath == inTexPath)
		{
			outTex = loadedTexture;
			return true;
		}
	}

	return false;
}
