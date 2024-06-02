#include "Renderer/Material/EngineMaterials/RenderMaterial_WithShadow.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "Renderer/RHI/RHI.h"

#include "Math/SphericalHarmonics.h"

RenderMaterial_WithShadow::RenderMaterial_WithShadow() = default;
RenderMaterial_WithShadow::~RenderMaterial_WithShadow() = default;

const eastl::string lsMatrixName = "lsMatrix";

void RenderMaterial_WithShadow::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

	// Forward Renderer

// 	eastl::vector<UniformWithFlag> defaultUniforms = {
// 	{"lsMatrices", 3},
// 	{"ShadowCameraViewMatrix"},
// 	{"cascadesCount"},
// 	{"shadowCascadeFarPlanes", 3},
// 	};
// 
// 	UBuffers.push_back({ defaultUniforms, ConstantBufferBinding::Pixel });
// 
// 
	// SH Lighting 

	//eastl::vector<UniformWithFlag> GIUniforms = {
	//{"LightCoeffs", SH_COEFFICIENT_COUNT},
	//};

	//UBuffers.push_back({ GIUniforms, EShaderType::Sh_Vertex });


	// ~SH Lighting


	// ~Forward Renderer

	eastl::vector<UniformWithFlag> LightingUniforms = {
		{"bHasNormalMap"},
		{"bOverrideColor"},
		{"OverrideColor"},
	};

	UBuffers.push_back({ LightingUniforms, EShaderType::Sh_Fragment });

// 
// 	eastl::vector<UniformWithFlag> debugConstants = {
// 	{"bShadowVisualizeMode"},
// 	{"bNormalVisualizeMode"},
// 	{"bUseNormalMapping"},
// 	{"bUseShadows"},
// 	};
// 
// 	UBuffers.push_back({ debugConstants, ConstantBufferBinding::Pixel });
}

void RenderMaterial_WithShadow::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
 	inUniformsCache["bHasNormalMap"] = OwnedTextures.size() > 1;
// 
// 	const eastl::weak_ptr<RHITexture2D> depthTexture = inUniformsCache["DirectionalLightCascadedShadowTexture"].GetValue<eastl::shared_ptr<RHITexture2D>>();
// 	
// 	if (eastl::find_if(ExternalTextures.begin(), ExternalTextures.end(), [depthTexture](const eastl::weak_ptr<RHITexture2D>& inPtr) {
// 		return depthTexture.lock() == inPtr.lock(); 
// 		}) == ExternalTextures.end())
// 	{
// 		ExternalTextures.push_back(depthTexture);
// 	}

	__super::SetUniformsValue(inUniformsCache, inShaderTypes);

// 
//  	UniformBufferContainer& container = UBuffers[1].BufferContainer;
//  	eastl::shared_ptr<RHIUniformBuffer>& RHIBuffer = container.RHIBuffer;
//  
//  	void* data = new char[container.Counter];
//  
//  	RHI::Get()->ReadBufferData(*RHIBuffer, 0, container.Counter, data);
//  
//  	SPointLight* test = static_cast<SPointLight*>(data);

}

