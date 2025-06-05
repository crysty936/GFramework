#pragma once
#include <utility>
#include "EASTL/vector.h"


class BindlessDecalsPass
{
public:
	BindlessDecalsPass();
	~BindlessDecalsPass();

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, struct SceneTextures& inSceneTextures);
	void ComputeTiledBinning(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures);
	void ComputeDecals(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures);
	void UpdateBeforeExecute();





};







