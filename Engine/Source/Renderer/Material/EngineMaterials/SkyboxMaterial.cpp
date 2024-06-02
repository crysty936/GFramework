#include "Renderer/Material/EngineMaterials/SkyboxMaterial.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"

SkyboxMaterial::SkyboxMaterial() = default;
SkyboxMaterial::~SkyboxMaterial() = default;

void SkyboxMaterial::SetRequiredUniforms()
{
// 	RequiredUniforms = {
// 		{"projection"},
// 		{"view"},
// 		};
}

void SkyboxMaterial::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
// 	const eastl::string uniformName = "view";
// 	auto& iter = inUniformsCache.find(uniformName);
// 
// 	if (iter == inUniformsCache.end())
// 	{
// 		ASSERT(false);
// 
// 		return;
// 	}
// 
// 	UniformWithFlag* requiredUniform = FindRequiredUniform(uniformName);
// 
//  	if (!requiredUniform)
//  	{
//  		return;
//  	}
// 
// 	glm::mat4& view = (*iter).second.Value.Value4fv;
// 	// Remove translation by taking the upper 3x3 of the matrix then casting to 4x4
// 	view = glm::mat4(glm::mat3(view));
// 	
// 	__super::SetUniforms(inUniformsCache);
}

