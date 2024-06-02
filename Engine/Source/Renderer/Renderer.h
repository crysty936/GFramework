#pragma once
#include "Window/WindowProperties.h"
#include "RenderCommand.h"
#include "SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "EASTL/unordered_map.h"
#include "EASTL/string.h"
#include "RHI/Resources/MeshDataContainer.h"

class Renderer
{
protected:
	Renderer(const WindowProperties& inMainWindowProperties);
	virtual ~Renderer();

public:
	// Will create the base window and return the context for it
	static void Init(const WindowProperties& inMainWindowProperties = {});
	static void Terminate();
	//virtual void Draw() = 0;
	//virtual void Present() = 0;

	inline static Renderer& Get() { ASSERT(Instance); return *Instance; }

protected:
	virtual void InitInternal() = 0;

protected:
	inline static Renderer* Instance = nullptr;

	const float CAMERA_FOV = 45.f;
	const float CAMERA_NEAR = 0.1f;
	const float CAMERA_FAR = 10000.f;
};

