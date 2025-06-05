#pragma once
#include <utility>
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"

struct SceneTextures
{
	eastl::shared_ptr<class D3D12RenderTarget2D> GBufferAlbedo;
	eastl::shared_ptr<class D3D12RenderTarget2D> GBufferNormal;
	eastl::shared_ptr<class D3D12RenderTarget2D> GBufferRoughness;
	eastl::shared_ptr<class D3D12DepthBuffer> MainDepthBuffer;
};

class DeferredBasePass
{
public:

	DeferredBasePass() = default;
	~DeferredBasePass() = default;

	void Init();
	void Execute(struct ID3D12GraphicsCommandList* inCmdList);

public:
	SceneTextures GBufferTextures;

};







