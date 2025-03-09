#pragma once
#include <utility>
#include "EASTL/vector.h"


struct SkyboxPass
{
	SkyboxPass();
	~SkyboxPass();


	void InitSkyModel();
	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, class D3D12RenderTarget2D& inRT, struct SceneTextures& inGBuffer);

};







