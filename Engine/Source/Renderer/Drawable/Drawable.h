#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "Entity/TransformObject.h"
#include "EASTL/unordered_map.h"

class DrawableObject : public TransformObject
{
public:
	DrawableObject(const eastl::string& inDrawableName);
	virtual ~DrawableObject();

	virtual glm::mat4 GetModelMatrix() const { return GetAbsoluteTransform().GetMatrix(); }

	inline void SetVisible(const bool inValue) { bIsVisible = inValue; }
	inline bool IsVisible() const { return bIsVisible; }

private:
	bool bIsVisible{ true };
};
