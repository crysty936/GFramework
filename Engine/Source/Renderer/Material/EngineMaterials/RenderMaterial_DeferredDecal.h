#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_DeferredDecal: public RenderMaterial
{
public:
	RenderMaterial_DeferredDecal();
	~RenderMaterial_DeferredDecal();

	virtual void SetRequiredUniforms() override;
	void SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes = Sh_Universal);
};