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
{}

AssimpModel3D::~AssimpModel3D() = default;

void AssimpModel3D::CreateProxy()
{

	// TODO: Make this work with multithreaded again
	//eastl::shared_ptr<AssimpModel3D> thisShared = this_shared(this);
	//RenderingLoadCommand loadCommand;
	//loadCommand.LoadDel = LoadRenderResourceDelegate::CreateStatic(LoadModelToRoot);
	//loadCommand.ModelPath = ModelPath;
	//loadCommand.Parent = thisShared;
	// Add load command
	
	LoadModelToRoot(ModelPath, shared_from_this());
}

void AssimpModel3D::LoadModelToRoot(const eastl::string inPath, TransformObjPtr inParent)
{
	eastl::vector<RenderCommand> resultingCommands;
	eastl::shared_ptr<MeshNode> mesh = LoadData(resultingCommands);

	if (ENSURE(mesh))
	{
		// @GFramework: TODO
		//Renderer::Get().AddCommands(resultingCommands);
		inParent->AddChild(mesh);
	}
}

eastl::shared_ptr<MeshNode> AssimpModel3D::LoadData(OUT eastl::vector<RenderCommand>& outCommands)
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
	ProcessNodesRecursively(*scene->mRootNode, *scene, newNode, outCommands);

	return newNode;
}

void AssimpModel3D::ProcessNodesRecursively(const aiNode & inNode, const aiScene & inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, OUT eastl::vector<RenderCommand>& outCommands)
{
	for (uint32_t i = 0; i < inNode.mNumMeshes; ++i)
	{
		const uint32_t meshIndex = inNode.mMeshes[i];
		const aiMesh* assimpMesh = inScene.mMeshes[meshIndex];

		ProcessMesh(*assimpMesh, inScene, inCurrentNode, outCommands);
	}

	for (uint32_t i = 0; i < inNode.mNumChildren; ++i)
	{
		const aiNode& nextAiNode = *inNode.mChildren[i];
		eastl::shared_ptr<MeshNode> newNode = eastl::make_shared<MeshNode>(nextAiNode.mName.C_Str());
		newNode->SetRelTransform(aiMatrixToTransform(nextAiNode.mTransformation));

		ProcessNodesRecursively(nextAiNode, inScene, newNode, outCommands);
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
	MaterialsManager& matManager = MaterialsManager::Get();
	eastl::shared_ptr<RenderMaterial> thisMaterial = matManager.GetOrAddMaterial<RenderMaterial_WithShadow>(eastl::string("Assimp_Material_") + inMesh.mName.data, outMatExists);

	return thisMaterial;
}

void AssimpModel3D::AddAdditionalBuffers(eastl::shared_ptr<MeshDataContainer>& inDataContainer) const
{
	// to be inherited for instancing, etc
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

void AssimpModel3D::ProcessMesh(const aiMesh& inMesh, const aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, OUT eastl::vector<RenderCommand>& outCommands)
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
 
 	if (!materialExists)
 	{
		eastl::vector<D3D12Texture2D> textures;
		if (inMesh.mMaterialIndex >= 0)
		{
			aiMaterial* Material = inScene.mMaterials[inMesh.mMaterialIndex];

			eastl::vector<eastl::shared_ptr<D3D12Texture2D>> diffuseMaps = LoadMaterialTextures(*Material, aiTextureType_DIFFUSE);
			thisMaterial->OwnedTextures.insert(thisMaterial->OwnedTextures.end(), eastl::make_move_iterator(diffuseMaps.begin()), eastl::make_move_iterator(diffuseMaps.end()));

			eastl::vector<eastl::shared_ptr<D3D12Texture2D>> normalMaps = LoadMaterialTextures(*Material, aiTextureType_NORMALS);
			thisMaterial->OwnedTextures.insert(thisMaterial->OwnedTextures.end(), eastl::make_move_iterator(normalMaps.begin()), eastl::make_move_iterator(normalMaps.end()));

  			eastl::vector<eastl::shared_ptr<D3D12Texture2D>> metRoughness = LoadMaterialTextures(*Material, aiTextureType_UNKNOWN);//AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE
 			thisMaterial->OwnedTextures.insert(thisMaterial->OwnedTextures.end(), eastl::make_move_iterator(metRoughness.begin()), eastl::make_move_iterator(metRoughness.end()));
		}

		thisMaterial->Shader = CreateShaders(inputLayout);
 	}
 
	const eastl::string renderDataContainerID = inMesh.mName.C_Str();
	eastl::shared_ptr<MeshDataContainer> dataContainer;

	ASSERT(0);
	//const bool existingContainer = Renderer::Get().GetOrCreateContainer(renderDataContainerID, dataContainer);

	eastl::vector<PathTraceTriangle> triangles;
	eastl::vector<Vertex> vertices;

	// @GFramework: TODO
	//if (!existingContainer)
	{
		eastl::vector<uint32_t> indices;

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
		eastl::shared_ptr<D3D12IndexBuffer> ib = D3D12RHI::Get()->CreateIndexBuffer(indices.data(), indicesCount);
		const int32_t verticesCount = static_cast<int32_t>(vertices.size());


		const eastl::shared_ptr<D3D12VertexBuffer> vb = D3D12RHI::Get()->CreateVertexBuffer(inputLayout, (float*)vertices.data(), vertices.size(), ib);

		dataContainer->VBuffer = vb;

		// For Pathtracer
// 		{
// 			for (int32_t i = 0; i < indicesCount; i += 3)
// 			{
// 				glm::vec3 v[3];
// 
// 				const size_t vertexSize = sizeof(glm::vec3);
// 				const size_t uvSize = sizeof(glm::vec2);
// 
// 				for (int32_t j = 0; j < 3; ++j)
// 				{
// 					constexpr int32_t stride = 14; // 3 floats for Pos vec3, 3 for Normal vec3 and 2 for UVs vec2, 3 for Tangent and 3 for Bitangent
// 
// 					const int32_t index = indices[i + j];
// 					const float* startPos = &vertices[0].Position.x + index * stride;
// 					memcpy(&v[j], startPos, vertexSize);
// 				}
// 
// 				PathTraceTriangle pathTraceTriangle = PathTraceTriangle(v);
// 
// 				triangles.push_back(pathTraceTriangle);
// 			}
// 		}
	}

	AddAdditionalBuffers(dataContainer);

	eastl::shared_ptr<MeshNode> newMesh = eastl::make_shared<MeshNode>(inMesh.mName.C_Str());
	inCurrentNode->AddChild(newMesh);

	RenderCommand newCommand = CreateRenderCommand(thisMaterial, inCurrentNode, dataContainer);
	newCommand.Triangles = std::move(triangles);
	newCommand.Vertices = std::move(vertices);
	outCommands.push_back(newCommand);
}

eastl::vector<eastl::shared_ptr<D3D12Texture2D>> AssimpModel3D::LoadMaterialTextures(const aiMaterial& inMat, const aiTextureType& inAssimpTexType)
{
	eastl::vector<eastl::shared_ptr<D3D12Texture2D>> textures;
	const uint32_t texturesCount = inMat.GetTextureCount(inAssimpTexType);

	for (uint32_t i = 0; i < texturesCount; ++i)
	{
		aiString Str;
		inMat.GetTexture(inAssimpTexType, i, &Str);
		eastl::shared_ptr<D3D12Texture2D> tex = nullptr;

		if (!IsTextureLoaded(Str.C_Str(), tex))
		{
			const eastl::string path = ModelDir + eastl::string("/") + eastl::string(Str.C_Str());

			ASSERT(0);
			//tex = D3D12RHI::Get()->CreateAndLoadTexture2D(path, inAssimpTexType != aiTextureType_NORMALS);
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
