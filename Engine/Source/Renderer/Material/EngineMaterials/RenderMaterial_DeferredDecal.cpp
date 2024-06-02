#include "Renderer/Material/EngineMaterials/RenderMaterial_DeferredDecal.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "Renderer/DrawDebugHelpers.h"
#include "Core/EngineCore.h"
#include "Window/WindowProperties.h"
#include "Window/WindowsWindow.h"

RenderMaterial_DeferredDecal::RenderMaterial_DeferredDecal() = default;
RenderMaterial_DeferredDecal::~RenderMaterial_DeferredDecal() = default;

void RenderMaterial_DeferredDecal::SetRequiredUniforms()
{
	RenderMaterial::SetRequiredUniforms();

	eastl::vector<UniformWithFlag> additionalUniforms = {
	{"perspInv"},
	{"viewInv"},
	{"CameraNearFar"},
	{"ViewportSize"},
	};

	UBuffers.push_back({ additionalUniforms, EShaderType::Sh_Fragment });
}

void RenderMaterial_DeferredDecal::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
	inUniformsCache["perspInv"] = glm::inverse(inUniformsCache["projection"].GetValue<glm::mat4>());
	inUniformsCache["viewInv"] = glm::inverse(inUniformsCache["view"].GetValue<glm::mat4>());

	const WindowsWindow& currentWindow = GEngine->GetMainWindow();
	const WindowProperties& props = currentWindow.GetProperties();
	inUniformsCache["ViewportSize"] = glm::vec4(props.Width, props.Height, 0.f, 0.f);

	__super::SetUniformsValue(inUniformsCache, inShaderTypes);
}

