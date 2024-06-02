#pragma once

#include "Core/EngineUtils.h"
#include "EASTL/array.h"
#include "EASTL/vector.h"
#include "glm/ext/vector_float3.hpp"

struct AABB
{
	glm::vec3 Min;
	glm::vec3 Max;

	AABB& operator +=(const AABB& inAABB);
	AABB& operator +=(const glm::vec3& inVec);

	inline glm::vec3 GetExtent() const
	{
		return 0.5f * (Max - Min);
	}

	inline void GetCenterAndExtent(OUT glm::vec3& outCenter, OUT glm::vec3& outExtent) const
	{
		outExtent = GetExtent();
		outCenter = Min + outExtent;
	}

	eastl::array<glm::vec3, 8> GetVertices() const;

	void DebugDraw() const;

private:
	bool IsInitialized = false;
};