#pragma once
#include "Renderer.h"
#include "EASTL/unique_ptr.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/vector.h"
#include "EASTL/unordered_map.h"
#include "EASTL/string.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "Math/Transform.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "EventSystem/EventSystem.h"
#include "EASTL/queue.h"
#include "Core/EngineUtils.h"
#include "Entity/TransformObject.h"
#include "Renderer/RHI/Resources/MeshDataContainer.h"
#include "Window/WindowProperties.h"
#include "RenderCommand.h"

class DeferredRenderer : public Renderer
{
public:
	DeferredRenderer(const WindowProperties& inWindowProps);
	virtual ~DeferredRenderer();

private:
	void InitInternal();
};

