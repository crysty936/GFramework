#pragma once
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "Core/EngineUtils.h"
#include "EASTL/array.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "AABB.h"

struct PathTracingRay
{
	glm::vec3 Origin = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 Direction = glm::vec3(0.f, 0.f, 0.f);
};

struct PathTracePayload
{
	float Distance = INFINITY;
	float U;
	float V;
	const struct PathTraceTriangle* Triangle = nullptr;
	//glm::vec3 Normal;
};

struct PathTraceTriangle
{
	PathTraceTriangle(glm::vec3 inVerts[3]);

	glm::vec3 V[3];
	glm::vec3 E[2];

	// This should not be normalised
	glm::vec3 WSNormal;
	glm::vec3 WSNormalNormalized;

	void Transform(const glm::mat4& inMatrix);
	AABB GetBoundingBox() const;
};

bool TraceTriangle(const PathTracingRay& inRay, const PathTraceTriangle& inTri, OUT PathTracePayload& outPayload);
bool IntersectsTriangle(const PathTracingRay& inRay, const PathTraceTriangle& inTri);
