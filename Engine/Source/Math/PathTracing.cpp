#include "Math/PathTracing.h"

PathTraceTriangle::PathTraceTriangle(glm::vec3 inVerts[3])
{
	V[0] = inVerts[0];
	V[1] = inVerts[1];
	V[2] = inVerts[2];

	E[0] = V[1] - V[0];
	E[1] = V[2] - V[0];

	WSNormal = glm::cross(E[0], E[1]);
	WSNormalNormalized = glm::normalize(WSNormal);
}

void PathTraceTriangle::Transform(const glm::mat4& inMatrix)
{
	for (int32_t i = 0; i < 3; ++i)
	{
		glm::vec3& currentVert = V[i];

		const glm::vec4 fourRowVec = glm::vec4(currentVert.x, currentVert.y, currentVert.z, 1.f);
		const glm::vec4 res = inMatrix * fourRowVec;

		currentVert = glm::vec3(res.x, res.y, res.z);
	}

	E[0] = V[1] - V[0];
	E[1] = V[2] - V[0];

	WSNormal = glm::cross(E[0], E[1]);
	WSNormalNormalized = glm::normalize(WSNormal);
}

AABB PathTraceTriangle::GetBoundingBox() const
{
	AABB res;

	for (int32_t i = 0; i < 3; ++i)
	{
		res += V[i];
	}

	return res;
}

bool TraceTriangle(const PathTracingRay& inRay, const PathTraceTriangle& inTri, OUT PathTracePayload& outPayload)
{
	const glm::vec3& A = inTri.V[0];
	const glm::vec3& E1 = inTri.E[0];
	const glm::vec3& E2 = inTri.E[1];
	const glm::vec3& N = inTri.WSNormal;

	float det = -dot(inRay.Direction, N);
	float invdet = 1.f / det;

	glm::vec3 AO = inRay.Origin - A;
	glm::vec3 DAO = glm::cross(AO, inRay.Direction);

	outPayload.U = dot(E2, DAO) * invdet;
	outPayload.V = -dot(E1, DAO) * invdet;
	outPayload.Distance = dot(AO, N) * invdet;
	outPayload.Triangle = &inTri;
	//outPayload.Normal = inTri.WSNormalNormalized;

	return (det >= 1e-6 && outPayload.Distance >= 0.0 && outPayload.U >= 0.0 && outPayload.V >= 0.0 && (outPayload.U + outPayload.V) <= 1.0);
}

bool IntersectsTriangle(const PathTracingRay& inRay, const PathTraceTriangle& inTri)
{
	PathTracePayload payload;
	return TraceTriangle(inRay, inTri, payload);
}

