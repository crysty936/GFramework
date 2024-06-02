#include "Renderer/Material/EngineMaterials/BallTestMaterial.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"

BallTestMaterial::BallTestMaterial() = default;
BallTestMaterial::~BallTestMaterial() = default;

void BallTestMaterial::SetUniformsValue(eastl::unordered_map<eastl::string, SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
	glm::vec3 lightPos(10.f, 20.f, 15.f);
	static float xRotation = 0.0f;
	xRotation += 0.05f;
	inUniformsCache["XRotation"] = SelfRegisteringUniform(xRotation);

	inUniformsCache["LightPos"] = SelfRegisteringUniform(lightPos);

	const glm::mat4& proj = inUniformsCache["projection"].GetValue<glm::mat4>();
	const glm::mat4& view = inUniformsCache["view"].GetValue<glm::mat4>();

	const glm::mat4 invProjView = glm::inverse((proj * view));

	inUniformsCache["ScreenToWorld"] = SelfRegisteringUniform(invProjView);

 	RenderMaterial::SetUniformsValue(inUniformsCache, inShaderTypes);
}

void BallTestMaterial::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

  	eastl::vector<UniformWithFlag> additionalUniforms = {
  	{"LightPos"},
  	{"ScreenToWorld"},
	{"XRotation"},
  	};
  
	UBuffers.push_back({ additionalUniforms, EShaderType::Sh_Fragment });
}
