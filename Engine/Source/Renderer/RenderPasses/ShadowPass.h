#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "glm/fwd.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "Utils/InlineVector.h"

#define NUM_CASCADES_MAX 6

class ShadowPass
{
public:
	ShadowPass() = default;
	~ShadowPass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, const glm::vec3& inLightDir);

	eastl::shared_ptr<class D3D12DepthBuffer> ShadowDepthBuffer;

private:
	vectorInline<glm::mat4, NUM_CASCADES_MAX> CreateCascadesMatrices(const glm::vec3& inLightDir, const class Scene& inCurrentScene) const;

private:
	int32_t NumCascades = 4;
	mutable glm::mat4 CachedCameraShadowMatrix;
	mutable bool bUpdateShadowValues = true;
	mutable bool bDrawCascadesProjection = false;
	mutable bool bDrawCascadesCameraFrustums = false;
};







