#include "Renderer/Material/EngineMaterials/RenderMaterial_LightSource.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "Renderer/DrawDebugHelpers.h"

RenderMaterial_LightSource::RenderMaterial_LightSource() = default;
RenderMaterial_LightSource::~RenderMaterial_LightSource() = default;

void RenderMaterial_LightSource::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

	eastl::vector<UniformWithFlag> additionalUniforms = {
	{"LightColor"},
	};

	UBuffers.push_back({ additionalUniforms, EShaderType::Sh_Fragment });
}
