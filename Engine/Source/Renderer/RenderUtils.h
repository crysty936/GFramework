#pragma once
#include "EASTL/array.h"
#include "glm/detail/type_vec3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "Drawable/Drawable.h"
#include "Model/3D/Model3D.h"

namespace RenderUtils
{
	eastl::array<glm::vec3, 8> GenerateSpaceCorners(const glm::mat4& SpaceToProjectionSpace, const float MinZ = 0.f, const float MaxZ = 1.f);
	glm::vec3 GetProjectionCenter(const glm::mat4& inProj);
}
