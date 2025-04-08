#pragma once
#include "glm/common.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include <stdlib.h>

const float PI = 3.14159265359f;

namespace MathUtils
{
	inline int32_t GetRandomInRange(int32_t inMinValue, int32_t inMaxValue) { return glm::max<int32_t>(inMinValue, (rand() % inMaxValue)); }

	inline uint32_t DivideAndRoundUp(uint32_t Dividend, uint32_t Divisor)
	{
		return (Dividend + Divisor - 1) / Divisor;
	}

	glm::mat4 BuildLookAt(const glm::vec3& inEyeDirection, const glm::vec3& inEyePos, const glm::vec3& inUp = glm::vec3(0.f, 1.f, 0.f));
}