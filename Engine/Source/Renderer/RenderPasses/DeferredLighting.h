#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"

struct DeferredLighting
{
	DeferredLighting();
	~DeferredLighting();


	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, struct SceneTextures& inSceneTextures);

	void RenderLighting(struct ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures);
};







