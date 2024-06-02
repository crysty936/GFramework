#include "Renderer/Drawable/Drawable.h"

DrawableObject::DrawableObject(const eastl::string& inDrawableName)
	: TransformObject(inDrawableName)
{

}
DrawableObject::~DrawableObject() = default;
