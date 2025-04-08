#include "RenderUtils.h"
#include "EASTL/string.h"
#include "RHI/Resources/VertexInputLayout.h"
#include "Drawable/ShapesUtils/BasicShapesData.h"
#include "RHI/RHITypes.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"

vectorInline<glm::vec3, 8> RenderUtils::GenerateSpaceCorners(const glm::mat4& SpaceToProjectionSpace, const float MinZ, const float MaxZ)
{
	const glm::vec3 ProjectionSpaceCorners[8] =
	{
		glm::vec3(1.0f, 1.0f, MinZ),
		glm::vec3(-1.0f, 1.0f, MinZ),
		glm::vec3(1.0f, -1.0f, MinZ),
		glm::vec3(-1.0f, -1.0f, MinZ),
		glm::vec3(1.0f, 1.0f, MaxZ),
		glm::vec3(-1.0f, 1.0f, MaxZ),
		glm::vec3(1.0f, -1.0f, MaxZ),
		glm::vec3(-1.0f, -1.0f, MaxZ)
	};

	const glm::mat4 ProjectionToSpace = glm::inverse(SpaceToProjectionSpace);
	vectorInline<glm::vec3, 8> SpaceCorners;

	for (int i = 0; i < 8; ++i)
	{
		const glm::vec3& currentCorner = ProjectionSpaceCorners[i];
		const glm::vec4 worldPos = ProjectionToSpace * glm::vec4(currentCorner.x, currentCorner.y, currentCorner.z, 1.f);
		SpaceCorners.push_back(worldPos / worldPos.w);
	}

	return SpaceCorners;
}

glm::vec3 RenderUtils::GetProjectionCenter(const glm::mat4& inProj)
{
	vectorInline<glm::vec3, 8> projCorners = GenerateSpaceCorners(inProj);

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const glm::vec3& v : projCorners)
	{
		center += glm::vec3(v);
	}
	center /= projCorners.size();

	return center;
}

uint32_t RenderUtils::ConvertToRGBA8(const glm::vec4& inColor)
{
	uint8_t r = (uint8_t)(inColor.r * 255.0f);
	uint8_t g = (uint8_t)(inColor.g * 255.0f);
	uint8_t b = (uint8_t)(inColor.b * 255.0f);
	uint8_t a = (uint8_t)(inColor.a * 255.0f);

	uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
	return result;
}
