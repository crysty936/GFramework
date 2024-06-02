#include "Renderer/Material/EngineMaterials/RenderMaterial_SphereHarmonicsDebug.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "Renderer/RHI/RHI.h"
#include "Math/SphericalHarmonics.h"
#include "imgui.h"

RenderMaterial_SphereHarmonicsDebug::RenderMaterial_SphereHarmonicsDebug()
{
	Samples = new SHSample[SH_TOTAL_SAMPLE_COUNT];

	SphericalHarmonics::InitSamples(Samples);
};
RenderMaterial_SphereHarmonicsDebug::~RenderMaterial_SphereHarmonicsDebug() = default;

void RenderMaterial_SphereHarmonicsDebug::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

	eastl::vector<UniformWithFlag> additionalUniforms = {
	{"ObjPos"},
	{"Theta"},
	{"Phi"},
	};

	UBuffers.push_back({ additionalUniforms, EShaderType::Sh_Fragment });
}

void RenderMaterial_SphereHarmonicsDebug::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
	ImGui::Begin("Spherical Harmonics Debug Stuff");

	static float theta = 0.f;
	ImGui::DragFloat("Sphere Theta", &theta, 0.01f, 0.f, 1.f);

	static float phi = 0.f;
	ImGui::DragFloat("Sphere Phi", &phi, 0.01f, 0.f, 2.f);


	inUniformsCache["Theta"] = theta;
	inUniformsCache["Phi"] = phi;

	__super::SetUniformsValue(inUniformsCache, inShaderTypes);

	ImGui::End();
}

