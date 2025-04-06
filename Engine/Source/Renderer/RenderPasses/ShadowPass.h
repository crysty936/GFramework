#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "glm/fwd.hpp"

struct ShadowPass
{
	ShadowPass() = default;
	~ShadowPass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, const glm::vec3& inLightDir);

	eastl::shared_ptr<class D3D12DepthBuffer> ShadowDepthBuffer;
};







