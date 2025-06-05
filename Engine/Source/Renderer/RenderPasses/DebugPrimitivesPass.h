#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"

class DebugPrimitivesPass
{
public:
	DebugPrimitivesPass() = default;
	~DebugPrimitivesPass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, const class D3D12DepthBuffer& inDepthBuffer, const class D3D12RenderTarget2D& inTarget);

};










