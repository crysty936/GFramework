#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_LightSource : public RenderMaterial
{
public:
	RenderMaterial_LightSource();
	~RenderMaterial_LightSource();

	virtual void SetRequiredUniforms() override;
};