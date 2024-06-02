#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "EASTL/vector.h"
#include "Utils/InlineVector.h"
#include "EASTL/array.h"

struct DebugLine
{
	glm::vec3 Start = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 End = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 Color = glm::vec3(0.f, 0.f, 0.f);
};

struct DebugPoint
{
	glm::vec3 Location = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 Color = glm::vec3(0.f, 0.f, 0.f);
	float Size = 1.f;
	bool bPersistent;
};

class DrawDebugManager
{

public:
	static void Draw();

private:
	static DrawDebugManager& Get()
	{
		static DrawDebugManager Instance;

		return Instance;
	}

	void AddDebugPoint(const glm::vec3& inPoint, const glm::vec3& inColor, const float inSize, const bool inPersistent);
	void AddDebugLine(const DebugLine& inLine);

private:
	
	void ClearDebugData();

private:
	eastl::vector<DebugPoint> DebugPoints;
	eastl::vector<DebugLine> DebugLines;

	friend class ForwardRenderer;
	friend class DeferredRenderer;
	friend struct DrawDebugHelpers;
};

struct DrawDebugHelpers
{
	static void DrawDebugPoint(const glm::vec3& inPoint, const float inSize = 1.f, const glm::vec3& inColor = glm::vec3(1.f, 1.f, 0.f), const bool inPersistent = false);
	static void DrawDebugLine(const DebugLine& inLine);
	static void DrawDebugLine(const glm::vec3& inStart, const glm::vec3& inEnd, const glm::vec3& inColor);
	static void DrawProjectionPoints(const glm::mat4& inProj);
	static void DrawLinesArray(const eastl::vector<glm::vec3>& inLinesPoints, const glm::vec3& inColor = glm::vec3(0.5f, 0.5f, 0.f));
	static void DrawBoxArray(eastl::array<glm::vec3, 8> inArray, const bool inDrawSides = true, const glm::vec3& inColor = glm::vec3(0.5f, 0.5f, 0.f));
	static void DrawProjection(const glm::mat4& inProj);
};

