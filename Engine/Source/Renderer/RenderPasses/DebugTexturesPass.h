#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "glm/fwd.hpp"
#include "glm/ext/matrix_float4x4.hpp"

class DebugTexturePass
{
public:
	DebugTexturePass() = default;
	~DebugTexturePass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, const class D3D12RenderTarget2D& inTarget);

private:
	void DrawTexture(struct ID3D12GraphicsCommandList* inCmdList, const class D3D12RenderTarget2D& inTarget);

private:
	glm::mat4 QuadTransform;
	glm::vec2 Offset	= glm::vec2(0.67f, -0.79f);
	glm::vec2 Scale		= glm::vec2(0.08f, 0.13f);
};







