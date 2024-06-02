#include "Renderer/Material/EngineMaterials/RenderMaterial_Billboard.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "Renderer/DrawDebugHelpers.h"

RenderMaterial_Billboard::RenderMaterial_Billboard() = default;
RenderMaterial_Billboard::~RenderMaterial_Billboard() = default;

void RenderMaterial_Billboard::SetRequiredUniforms()
{
	__super::SetRequiredUniforms();

	eastl::vector<UniformWithFlag> additionalUniforms = {
	{"cameraLookAt"},
	};

	UBuffers.push_back({ additionalUniforms, EShaderType::Sh_Vertex });
}

void RenderMaterial_Billboard::SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes)
{
	const glm::vec3 cameraPos = inUniformsCache["ViewPos"].GetValue<glm::vec3>();
	const glm::vec3 objPos = inUniformsCache["ObjPos"].GetValue<glm::vec3>();

	const glm::vec3 upVec(0, 1, 0);

	const glm::vec3 z_axis = glm::normalize(glm::vec3(cameraPos.x, objPos.y, cameraPos.z) - objPos);
	const glm::vec3 x_axis = glm::normalize(glm::cross(upVec, z_axis));
	const glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis));

	const glm::mat4 rotationMatrix(
		glm::vec4(x_axis, 0),
		glm::vec4(y_axis, 0),
		glm::vec4(z_axis, 0),
		glm::vec4(0, 0, 0, 1));

	inUniformsCache["cameraLookAt"] = rotationMatrix;

	__super::SetUniformsValue(inUniformsCache, inShaderTypes);
}

