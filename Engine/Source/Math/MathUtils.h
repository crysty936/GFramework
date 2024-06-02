#pragma once
#include "glm/common.hpp"

const float PI = 3.14159265359f;

namespace MathUtils
{
	inline int GetRandom(int32_t inMinValue, int32_t inMaxValue) { return glm::max(inMinValue, (rand() % inMaxValue)); }
}