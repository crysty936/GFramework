#include "Renderer/Material/EngineMaterials/RenderMaterial_Parallax.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "imgui.h"

RenderMaterial_Parallax::RenderMaterial_Parallax() = default;
RenderMaterial_Parallax::~RenderMaterial_Parallax() = default;

void RenderMaterial_Parallax::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

	eastl::vector<UniformWithFlag> defaultUniforms = {
	{"DirectionalLightDirection"},
	{"bNormalVisualizeMode"},
	{"bUseNormalMapping"},
	{"bUseParallaxMapping"},
	{"ParallaxHeightScale"},
	{"ViewPos"},
	};

	UBuffers.push_back({ defaultUniforms, EShaderType::Sh_Vertex });
}

void RenderMaterial_Parallax::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
	ImGui::SeparatorText("Parallax");
	static bool bUseParallaxMapping = true;
	static float ParallaxHeightScale = 0.1f;
	ImGui::Checkbox("Use Parallax Mapping", &bUseParallaxMapping);
	inUniformsCache["bUseParallaxMapping"] = bUseParallaxMapping ? 1 : 0;
	ImGui::DragFloat("Parallax Height Scale", &ParallaxHeightScale, 0.01f, 0.f, 1.f, "%f", ImGuiSliderFlags_AlwaysClamp);
	inUniformsCache["ParallaxHeightScale"] = ParallaxHeightScale;

	__super::SetUniformsValue(inUniformsCache, inShaderTypes);
}

