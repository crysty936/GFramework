#pragma once

#include "Core/EngineUtils.h"
#include "EASTL/array.h"
#include "EASTL/vector.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/quaternion_float.hpp"

struct SphericalHarmonicsRotation
{
	static void Init();
	static void Rotate(const glm::quat& inRot, const eastl::vector<glm::vec4>& inCoeffs, eastl::vector<glm::vec4>& outRotatedCoeffs);


};