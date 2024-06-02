#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Renderer/Material/RenderMaterial.h"

class RenderMaterial_Debug : public RenderMaterial
{
public:
	RenderMaterial_Debug();
	~RenderMaterial_Debug();

	virtual void SetRequiredUniforms() override;
};