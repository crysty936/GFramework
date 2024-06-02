#include "Renderer/Material/EngineMaterials/RenderMaterial_Debug.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"

RenderMaterial_Debug::RenderMaterial_Debug() = default;
RenderMaterial_Debug::~RenderMaterial_Debug() = default;

void RenderMaterial_Debug::SetRequiredUniforms()
{
	eastl::vector<UniformWithFlag> defaultUniforms = {
	{"projection"},
	{"view"},
	};

	UBuffers.push_back({ defaultUniforms, EShaderType::Sh_Vertex });
}
