#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"

struct DeferredLightingPass
{
	DeferredLightingPass();
	~DeferredLightingPass();


	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, struct SceneTextures& inSceneTextures, const class D3D12RenderTarget2D& inTarget);

	void RenderLighting(struct ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures, const class D3D12RenderTarget2D& inTarget);
};







