#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_DefaultLighting : public RenderMaterial
{
public:
	RenderMaterial_DefaultLighting();
	~RenderMaterial_DefaultLighting();

	virtual void SetRequiredUniforms() override;
	void SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes = Sh_Universal);
};