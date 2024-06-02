#pragma once
#include "Renderer/Model/3D/Model3D.h"
#include "EASTL/string.h"
#include "Core/EngineUtils.h"
#include "assimp/material.h"
#include "Renderer/RenderCommand.h"

class AssimpModel3D : public Model3D
{
public:
	AssimpModel3D(const eastl::string& inPath, const eastl::string& inName, glm::vec3 inOverrideColor = glm::vec3(1.f, 1.f, 1.f));
	virtual ~AssimpModel3D();

	virtual void CreateProxy() override;

	void LoadModelToRoot(const eastl::string inPath, TransformObjPtr inParent);

protected:
	virtual eastl::shared_ptr<RHIShader> CreateShaders(const class VertexInputLayout& inLayout) const;
	virtual eastl::shared_ptr<RenderMaterial> CreateMaterial(const struct aiMesh& inMesh, bool& outMatExists) const;
	virtual void AddAdditionalBuffers(eastl::shared_ptr<MeshDataContainer>& inDataContainer) const;
	virtual RenderCommand CreateRenderCommand(eastl::shared_ptr<RenderMaterial>& inMaterial, eastl::shared_ptr<MeshNode>& inParent, eastl::shared_ptr<MeshDataContainer>& inDataContainer);

	eastl::shared_ptr<MeshNode> LoadData(OUT eastl::vector<RenderCommand>& outCommands);
	void ProcessNodesRecursively(const struct aiNode& inNode, const struct aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, OUT eastl::vector<RenderCommand>& outCommands);
	void ProcessMesh(const struct aiMesh& inMesh, const struct aiScene& inScene, eastl::shared_ptr<MeshNode>& inCurrentNode, OUT eastl::vector<RenderCommand>& outCommands);

	eastl::vector<eastl::shared_ptr<class D3D12Texture2D>> LoadMaterialTextures(const struct aiMaterial& inMat, const aiTextureType& inAssimpTexType);
	bool IsTextureLoaded(const eastl::string& inTexPath, OUT eastl::shared_ptr<class D3D12Texture2D>& outTex);
private:
	eastl::vector<eastl::shared_ptr<class D3D12Texture2D>> LoadedTextures;
	eastl::string ModelDir;
	eastl::string ModelPath;
	glm::vec3 OverrideColor = glm::vec3(0.f, 0.f, 0.f);
};