#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"

struct DebugPrimitivesPass
{

	DebugPrimitivesPass() = default;
	~DebugPrimitivesPass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, const class D3D12RenderTarget2D& inTarget);

};










