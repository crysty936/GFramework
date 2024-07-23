#pragma once
#include "Renderer/DrawType.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/vector.h"
#include "Math/PathTracing.h"
#include "Math/BVH.h"
#include "RenderingPrimitives.h"

namespace EDrawMode
{
	enum Type : uint8_t
	{
		Default = 1 << 0,
		DEPTH = 1 << 2,
		DEPTH_VISUALIZE = 1 << 3,
		OUTLINE = 1 << 4,
		NORMAL_VISUALIZE = 1 << 5,
		Count = 1 << 6
	};
}

struct RenderCommand
{
	eastl::shared_ptr<class MeshDataContainer> DataContainer;
	eastl::shared_ptr<class RenderMaterial> Material;
	eastl::weak_ptr<const class DrawableObject> Parent;

	EDrawType DrawType = EDrawType::DrawElements;
	EDrawMode::Type DrawPasses = EDrawMode::Default;

	/** In case draw mode is DrawInstanced */
	int32_t InstancesCount = 0;

	glm::vec3 OverrideColor = glm::vec3(0.f, 0.f, 0.f);
};

