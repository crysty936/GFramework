#pragma once
#include <utility>
#include "EASTL/vector.h"


struct SkyboxPass
{
	SkyboxPass();
	~SkyboxPass();


	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList);

};







