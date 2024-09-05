#pragma once
#include "Renderer/Model/3D/Model3D.h"
#include "EASTL/string.h"
#include "Core/EngineUtils.h"
#include "assimp/material.h"

class AssimpModel3D : public Model3D
{
public:
	AssimpModel3D(const eastl::string& inPath, const eastl::string& inName, glm::vec3 inOverrideColor = glm::vec3(1.f, 1.f, 1.f));
	virtual ~AssimpModel3D();

	void LoadModelToRoot(const eastl::string inPath, TransformObjPtr inParent, struct ID3D12GraphicsCommandList* inCommandList);


	void Init(struct ID3D12GraphicsCommandList* inCommandList) override;

protected:
	eastl::shared_ptr<MeshNode> LoadData(struct ID3D12GraphicsCommandList* inCommandList);
	void ProcessMaterials(const struct aiScene& inScene, struct ID3D12GraphicsCommandList* inCommandList);
	void ProcessNodesRecursively(const struct aiNode& inNode, const struct aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, struct ID3D12GraphicsCommandList* inCommandList);
	void ProcessMesh(const struct aiMesh& inMesh, const struct aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, struct ID3D12GraphicsCommandList* inCommandList);

	eastl::shared_ptr<class D3D12Texture2D> LoadMaterialTexture(const struct aiMaterial& inMat, const aiTextureType& inAssimpTexType, struct ID3D12GraphicsCommandList* inCommandList);
	bool IsTextureLoaded(const eastl::string& inTexPath, OUT eastl::shared_ptr<class D3D12Texture2D>& outTex);
private:
	eastl::vector<eastl::shared_ptr<class D3D12Texture2D>> LoadedTextures;
	eastl::string ModelDir;
	eastl::string ModelPath;
	glm::vec3 OverrideColor = glm::vec3(0.f, 0.f, 0.f);
};