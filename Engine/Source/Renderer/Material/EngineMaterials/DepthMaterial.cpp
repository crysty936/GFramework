#include "Renderer/Material/EngineMaterials/DepthMaterial.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"
#include "Renderer/Material/MaterialHelpers.h"
#include "glm/ext/matrix_transform.inl"

DepthMaterial::DepthMaterial() = default;
DepthMaterial::~DepthMaterial() = default;

const eastl::string lsMatrixName = "lsMatrix";

void DepthMaterial::SetRequiredUniforms()
{
// 	eastl::vector<UniformWithFlag> defaultUniforms = {
// 	{"lsMatrix"},
// 	{"model"}
// 	};
// 
// 	UBuffers.push_back({ defaultUniforms, ConstantBufferType::Vertex });

  	eastl::vector<UniformWithFlag> vertexUniforms = {
  	{"model"}
  	};
  
	UBuffers.push_back({ vertexUniforms, EShaderType::Sh_Vertex });

	eastl::vector<UniformWithFlag> geometryUniforms = {
	{"lsMatrices", 3}
	};

	UBuffers.push_back({ geometryUniforms, EShaderType::Sh_Geometry });
}
