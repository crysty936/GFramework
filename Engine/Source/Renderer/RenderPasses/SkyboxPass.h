#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "glm/glm.hpp"
#include "EASTL/shared_ptr.h"

class SkyboxPass
{
public:
	SkyboxPass();
	~SkyboxPass();


	void InitSkyModel(struct ID3D12GraphicsCommandList* inCmdList);
	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList, class D3D12RenderTarget2D& inRT, struct SceneTextures& inGBuffer);

private:
	struct ArHosekSkyModelState* StateR = nullptr;
	struct ArHosekSkyModelState* StateG = nullptr;
	struct ArHosekSkyModelState* StateB = nullptr;

	eastl::shared_ptr<class D3D12Texture2D> Cubemap;
	
	glm::vec3 SunDirection = glm::vec3(0.25f, 0.95f, -0.15f);
	glm::vec3 GroundAlbedo = glm::vec3(0.25f, 0.25f, 0.25f);
	glm::vec3 SunDirectionCache;
	glm::vec3 GroundAlbedoCache;
	float Turbidity = 2.f;
	float TurbidityCache = 0.f;

	float SkyExposure = -14.f;

	bool bInitialized = false;
};







