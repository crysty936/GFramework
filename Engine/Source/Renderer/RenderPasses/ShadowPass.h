#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "glm/fwd.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "Utils/InlineVector.h"

#define MAX_NUM_CASCADES 3

struct ShadowPassOutput
{
	ShadowPassOutput(const class ShadowPass& inPass);

	//eastl::shared_ptr<class D3D12DepthBuffer> DepthBuffer;
	vectorInline<glm::mat4, MAX_NUM_CASCADES> CascadesMatrices;
};

class ShadowPass
{
public:
	ShadowPass() = default;
	~ShadowPass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, const glm::vec3& inLightDir);

	inline eastl::shared_ptr<class D3D12DepthBuffer> GetShadowDepthBuffer() const { return ShadowDepthBuffer; }
	inline vectorInline<glm::mat4, MAX_NUM_CASCADES> GetCascadesMatrices() const { return CascadesMatrices; }

	ShadowPassOutput GetOutput() const;

private:
	vectorInline<glm::mat4, MAX_NUM_CASCADES> CreateCascadesMatrices(const glm::vec3& inLightDir, const class Scene& inCurrentScene) const;

private:
	eastl::shared_ptr<class D3D12DepthBuffer> ShadowDepthBuffer;
	int32_t NumCascades = MAX_NUM_CASCADES;
	vectorInline<glm::mat4, MAX_NUM_CASCADES> CascadesMatrices;
	mutable glm::mat4 CachedCameraShadowMatrix;
	mutable bool bUpdateShadowValues = true;
	mutable bool bDrawCascadesProjection = false;
	mutable bool bDrawCascadesCameraFrustums = false;
};







