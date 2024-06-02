#include "Renderer/Material/EngineMaterials/RenderMaterial_PBRSphere.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "Renderer/RHI/RHI.h"
#include "imgui.h"

RenderMaterial_PBRSphere::RenderMaterial_PBRSphere() = default;
RenderMaterial_PBRSphere::~RenderMaterial_PBRSphere() = default;

void RenderMaterial_PBRSphere::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

		eastl::vector<UniformWithFlag> additionalUniforms = {
	{"EditorSphereMetallic"},
	{"EditorSphereRoughness"},
	};

	UBuffers.push_back({ additionalUniforms, EShaderType::Sh_Fragment });
}

void RenderMaterial_PBRSphere::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
	ImGui::Begin("Editor Sphere settings");

	static float EditorSphereMetallic = 0.f;
	ImGui::DragFloat("Editor Sphere Metallic", &EditorSphereMetallic, 0.01f, 0.f, 1.f);

	static float EditorSphereRoughess = 0.0f;
	ImGui::DragFloat("Editor Sphere Roughness", &EditorSphereRoughess, 0.01f, 0.f, 1.f);

	inUniformsCache["EditorSphereMetallic"] = EditorSphereMetallic;
	inUniformsCache["EditorSphereRoughness"] = EditorSphereRoughess;

	__super::SetUniformsValue(inUniformsCache, inShaderTypes);

	ImGui::End();
}

