#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class DepthMaterial : public RenderMaterial
{
public:
	DepthMaterial();
	~DepthMaterial();

	virtual void SetRequiredUniforms() override;
};