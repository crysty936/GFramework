#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_SphereHarmonicsDebug : public RenderMaterial
{
public:
	RenderMaterial_SphereHarmonicsDebug();
	~RenderMaterial_SphereHarmonicsDebug();

	virtual void SetRequiredUniforms() override;
	virtual void SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes = Sh_Universal) override;

private:
	struct SHSample* Samples = nullptr;
};