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
	vectorInline<float, MAX_NUM_CASCADES> CascadesFarPlanes;
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
	inline vectorInline<float, MAX_NUM_CASCADES> GetCascadesFarPlanes() const { return CascadesFarPlanes; }

	ShadowPassOutput GetOutput() const;

private:
	void CreateCascadesMatrices(const glm::vec3& inLightDir, const class Scene& inCurrentScene);

private:
	int32_t NumCascades = MAX_NUM_CASCADES;
	eastl::shared_ptr<class D3D12DepthBuffer> ShadowDepthBuffer;
	vectorInline<glm::mat4, MAX_NUM_CASCADES> CascadesMatrices;
	vectorInline<float, MAX_NUM_CASCADES> CascadesFarPlanes;
	mutable glm::mat4 CachedCameraShadowMatrix;
	mutable bool bUpdateShadowValues = true;
	mutable bool bDrawCascadesProjection = false;
	mutable bool bDrawCascadesCameraFrustums = false;
};







