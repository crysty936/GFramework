#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_PBRSphere : public RenderMaterial
{
public:
	RenderMaterial_PBRSphere();
	~RenderMaterial_PBRSphere();

	virtual void SetRequiredUniforms() override;
	virtual void SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes = Sh_Universal) override;
};