#include "AssimpModel3D.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "Logger/Logger.h"
#include "assimp/postprocess.h"
#include "Renderer/Material/MaterialsManager.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/RenderingPrimitives.h"
#include "Renderer/RHI/Resources/MeshDataContainer.h"
#include "Renderer/RHI/Resources/RHITexture.h"
#include "Renderer/RHI/RHI.h"
#include "Renderer/Material/EngineMaterials/RenderMaterial_WithShadow.h"
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
	eastl::vector<RenderCommand> resultingCommands;
	eastl::shared_ptr<MeshNode> mesh = LoadData(resultingCommands, inCommandList);

	if (ENSURE(mesh))
	{
		// @GFramework: TODO
		//Renderer::Get().AddCommands(resultingCommands);
		inParent->AddChild(mesh);
	}
}

void AssimpModel3D::Init(ID3D12GraphicsCommandList* inCommandList)
{
	LoadModelToRoot(ModelPath, shared_from_this(), inCommandList);
}

eastl::shared_ptr<MeshNode> AssimpModel3D::LoadData(OUT eastl::vector<RenderCommand>& outCommands, ID3D12GraphicsCommandList* inCommandList)
{
	Assimp::Importer modelImporter;

	const aiScene* scene = modelImporter.ReadFile(ModelPath.c_str(), 0);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		LOG_ERROR("Unable to load model from path %s", ModelPath.c_str());

		return nullptr;
	}

	ModelDir = ModelPath.substr(0, ModelPath.find_last_of('/'));

	eastl::shared_ptr<MeshNode> newNode = eastl::make_shared<MeshNode>("RootNode");
	newNode->SetRelTransform(aiMatrixToTransform(scene->mRootNode->mTransformation));
	ProcessNodesRecursively(*scene->mRootNode, *scene, newNode, inCommandList, outCommands);

	return newNode;
}

void AssimpModel3D::ProcessNodesRecursively(const aiNode & inNode, const aiScene & inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, struct ID3D12GraphicsCommandList* inCommandList, OUT eastl::vector<RenderCommand>& outCommands)
{
	for (uint32_t i = 0; i < inNode.mNumMeshes; ++i)
	{
		const uint32_t meshIndex = inNode.mMeshes[i];
		const aiMesh* assimpMesh = inScene.mMeshes[meshIndex];

		ProcessMesh(*assimpMesh, inScene, inCurrentNode, inCommandList, outCommands);
	}

	for (uint32_t i = 0; i < inNode.mNumChildren; ++i)
	{
		const aiNode& nextAiNode = *inNode.mChildren[i];
		eastl::shared_ptr<MeshNode> newNode = eastl::make_shared<MeshNode>(nextAiNode.mName.C_Str());
		newNode->SetRelTransform(aiMatrixToTransform(nextAiNode.mTransformation));

		ProcessNodesRecursively(nextAiNode, inScene, newNode, inCommandList, outCommands);
		inCurrentNode->AddChild((newNode));
	}
}

eastl::shared_ptr<RHIShader> AssimpModel3D::CreateShaders(const VertexInputLayout& inLayout) const
{
	eastl::vector<ShaderSourceInput> shaders = {
		{ "GenericAssimpModel/VS_Pos-UV-Normal-Tangent-Bitangent_Model_WorldPosition_WithShadow", EShaderType::Sh_Vertex },
		{ "GenericAssimpModel/PS_TexNormalMapped_WithShadow", EShaderType::Sh_Fragment } };

	ASSERT(0);

	//return RHI::Get()->CreateShaderFromPath(shaders, inLayout);

	return nullptr;
}

eastl::shared_ptr<RenderMaterial> AssimpModel3D::CreateMaterial(const aiMesh& inMesh, bool& outMatExists) const
{
	static eastl::set<eastl::string> potentiallyCreatedMaterials;
	auto iterator = potentiallyCreatedMaterials.find(eastl::string("Assimp_Material_") + inMesh.mName.data);
	//auto iterator = potentiallyCreatedMaterials.find_as(eastl::string("Assimp_Material_") + inMesh.mName.data, eastl::less_2<eastl::string, const char*>());
	const bool alreadyExists = iterator != potentiallyCreatedMaterials.end();

	if (alreadyExists)
	{
		ASSERT(false);
	}

	if (!alreadyExists)
	{
		potentiallyCreatedMaterials.insert(eastl::string("Assimp_Material_") + inMesh.mName.data);
	}

	//MaterialsManager& matManager = MaterialsManager::Get();
	//eastl::shared_ptr<RenderMaterial> thisMaterial = matManager.GetOrAddMaterial<RenderMaterial_WithShadow>(eastl::string("Assimp_Material_") + inMesh.mName.data, outMatExists);

	return nullptr;
}

RenderCommand AssimpModel3D::CreateRenderCommand(eastl::shared_ptr<RenderMaterial>& inMaterial, eastl::shared_ptr<MeshNode>& inParent, eastl::shared_ptr<MeshDataContainer>& inDataContainer)
{
	RenderCommand newCommand;
	newCommand.Material = inMaterial;
	newCommand.Parent = inParent;
	newCommand.DataContainer = inDataContainer;
	newCommand.DrawType = EDrawType::DrawElements;
	newCommand.DrawPasses = static_cast<EDrawMode::Type>(EDrawMode::Default /*| EDrawMode::NORMAL_VISUALIZE*/);
	newCommand.OverrideColor = OverrideColor;

	return newCommand;
}

void AssimpModel3D::ProcessMesh(const aiMesh& inMesh, const aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, ID3D12GraphicsCommandList* inCommandList, OUT eastl::vector<RenderCommand>& outCommands)
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

 	bool materialExists = false;
	eastl::shared_ptr<RenderMaterial> thisMaterial = CreateMaterial(inMesh, materialExists);
	eastl::vector<eastl::shared_ptr<D3D12Texture2D>> textures;

 	//if (!materialExists)
 	{
		//eastl::vector<D3D12Texture2D> textures;
		if (inMesh.mMaterialIndex >= 0)
		{
			aiMaterial* Material = inScene.mMaterials[inMesh.mMaterialIndex];

			eastl::vector<eastl::shared_ptr<D3D12Texture2D>> diffuseMaps = LoadMaterialTextures(*Material, aiTextureType_DIFFUSE, inCommandList);
			textures.insert(textures.end(), eastl::make_move_iterator(diffuseMaps.begin()), eastl::make_move_iterator(diffuseMaps.end()));

			eastl::vector<eastl::shared_ptr<D3D12Texture2D>> normalMaps = LoadMaterialTextures(*Material, aiTextureType_NORMALS, inCommandList);
			textures.insert(textures.end(), eastl::make_move_iterator(normalMaps.begin()), eastl::make_move_iterator(normalMaps.end()));

  			eastl::vector<eastl::shared_ptr<D3D12Texture2D>> metRoughness = LoadMaterialTextures(*Material, aiTextureType_UNKNOWN, inCommandList);//AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE
			textures.insert(textures.end(), eastl::make_move_iterator(metRoughness.begin()), eastl::make_move_iterator(metRoughness.end()));
		}

		//thisMaterial->Shader = CreateShaders(inputLayout);
 	}
 
	eastl::shared_ptr<D3D12IndexBuffer> indexBuffer;
	eastl::shared_ptr<D3D12VertexBuffer> vertexBuffer;

	//const eastl::string renderDataContainerName = inMesh.mName.C_Str();
	//const bool existingContainer = Renderer::Get().GetOrCreateContainer(renderDataContainerName, dataContainer);

	eastl::vector<Vertex> vertices;
	eastl::vector<uint32_t> indices;

	// @GFramework: TODO
	//if (!existingContainer)
	{

		for (uint32_t i = 0; i < inMesh.mNumVertices; i++)
		{
			Vertex vert;
			const aiVector3D& aiVertex = inMesh.mVertices[i];
			const aiVector3D& aiNormal = inMesh.mNormals[i];

			vert.Position = glm::vec3(aiVertex.x, aiVertex.y, aiVertex.z);
			vert.Normal = glm::vec3(aiNormal.x, aiNormal.y, aiNormal.z);
			if (inMesh.mTangents)
			{
				const aiVector3D& aiTangent = inMesh.mTangents[i];
				vert.Tangent = glm::vec3(aiTangent.x, aiTangent.y, aiTangent.z);
			}
			else
			{
				vert.Tangent = glm::vec3(1.f, 0.f, 0.f);
			}

			if (inMesh.mBitangents)
			{
				const aiVector3D& aiBitangent = inMesh.mBitangents[i];
				vert.Bitangent = glm::vec3(aiBitangent.x, aiBitangent.y, aiBitangent.z);
			}
			else
			{
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
	newMesh->Textures = textures;

	inCurrentNode->AddChild(newMesh);

	//RenderCommand newCommand = CreateRenderCommand(thisMaterial, inCurrentNode, dataContainer);
	//outCommands.push_back(newCommand);
}

eastl::vector<eastl::shared_ptr<D3D12Texture2D>> AssimpModel3D::LoadMaterialTextures(const aiMaterial& inMat, const aiTextureType& inAssimpTexType, ID3D12GraphicsCommandList* inCommandList)
{
	eastl::vector<eastl::shared_ptr<D3D12Texture2D>> textures;
	const uint32_t texturesCount = inMat.GetTextureCount(inAssimpTexType);

	for (uint32_t i = 0; i < texturesCount; ++i)
	{
		aiString Str;
		inMat.GetTexture(inAssimpTexType, i, &Str);
		eastl::shared_ptr<D3D12Texture2D> tex = nullptr;

		// Currently, textures in descriptor tables are assigned based on the first texture.
		// This means that there's a possibility for textures with similar albedo but diferences in others to render wrong/affect others down the pipe
		// TODO: Protect against this
		if (!IsTextureLoaded(Str.C_Str(), tex))
		{
			const eastl::string path = ModelDir + eastl::string("/") + eastl::string(Str.C_Str());

			tex = D3D12RHI::Get()->CreateAndLoadTexture2D(path, inAssimpTexType == aiTextureType_DIFFUSE, inCommandList);
			tex->SourcePath = eastl::string(Str.C_Str());
			LoadedTextures.push_back(tex);
		}

		textures.push_back(tex);
	}

	return textures;
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
