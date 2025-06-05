#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "glm/fwd.hpp"

class DeferredLightingPass
{
public:

	DeferredLightingPass() = default;
	~DeferredLightingPass() = default;

	void Init(struct SceneTextures& inSceneTextures);
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, struct SceneTextures& inSceneTextures, const class D3D12RenderTarget2D& inTarget, const glm::vec3& inLightDir);

private:
	void RenderLighting(struct ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures, const class D3D12RenderTarget2D& inTarget, const glm::vec3& inLightDir);
};







