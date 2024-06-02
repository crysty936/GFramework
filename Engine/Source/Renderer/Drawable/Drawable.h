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
	virtual void UpdateCustomUniforms(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache) const {};

	inline void SetVisible(const bool inValue) { bIsVisible = inValue; }
	inline bool IsVisible() const { return bIsVisible; }

	virtual void CreateProxy() {};

private:
	bool bIsVisible{ true };
};

// Drawable Container is the one that sets up the Render Commmands for the DrawableObjects
class IDrawableContainer
{
public:
	virtual ~IDrawableContainer() = default;

	virtual void CreateProxy() = 0;
};



