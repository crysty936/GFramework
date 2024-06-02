#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_Parallax : public RenderMaterial
{
public:
	RenderMaterial_Parallax();
	~RenderMaterial_Parallax();

	virtual void SetRequiredUniforms() override;

	void SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes = Sh_Universal) override;
};