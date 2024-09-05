#include "AssimpModel3DPBRSphere.h"
#include "assimp/mesh.h"

AssimpModel3DPBRSphere::AssimpModel3DPBRSphere(const eastl::string& inPath, const eastl::string& inName)
	: AssimpModel3D(inPath, inName)
{


}

AssimpModel3DPBRSphere::~AssimpModel3DPBRSphere() = default;

//eastl::shared_ptr<RHIShader> AssimpModel3DPBRSphere::CreateShaders(const class VertexInputLayout& inLayout) const
//{
//	eastl::vector<ShaderSourceInput> shaders = {
//	{ "EditorSphere/VS_EditorSphere", EShaderType::Sh_Vertex },
//	{ "EditorSphere/PS_EditorSphere", EShaderType::Sh_Fragment } };
//
//	//return RHI::Get()->CreateShaderFromPath(shaders, inLayout);
//
//	ASSERT(0);
//
//	return nullptr;
//}
//
//eastl::shared_ptr<RenderMaterial> AssimpModel3DPBRSphere::CreateMaterial(const aiMesh& inMesh, bool& outMatExists) const
//{
//	static int32_t instanceCount = 0;
//	++instanceCount;
//
//	MaterialsManager& matManager = MaterialsManager::Get();
//	eastl::shared_ptr<RenderMaterial> thisMaterial = matManager.GetOrAddMaterial<RenderMaterial_PBRSphere>(eastl::string("Assimp_EditorSphere_Material_") + inMesh.mName.data + eastl::to_string(instanceCount), outMatExists);
//
//	return thisMaterial;
//}

