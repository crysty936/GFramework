#pragma once

#include "Renderer/RHI/Resources/RHIBuffers.h"
#include <dxgiformat.h>
#include <d3d12.h>
#include "Renderer/RHI/Resources/RHITexture.h"
#include "D3D12Utility.h"

class D3D12IndexBuffer : public RHIIndexBuffer
{
public:
	D3D12IndexBuffer();

	D3D12_INDEX_BUFFER_VIEW IBView() const;

public:
	struct ID3D12Resource* Resource = nullptr;
	DXGI_FORMAT IBFormat = DXGI_FORMAT_UNKNOWN;
	uint64_t GPUAddress = 0;
	uint32_t Size = 0;
};

class D3D12VertexBuffer : public RHIVertexBuffer
{
public:
	D3D12VertexBuffer();

	D3D12_VERTEX_BUFFER_VIEW VBView() const;

public:
	struct ID3D12Resource* Resource = nullptr;
	uint64_t GPUAddress = 0;
	uint64_t NumElements = 0;


};

class D3D12Texture2D : public RHITexture2D
{
public:
	ID3D12Resource* Resource = nullptr;
	uint32_t SRVIndex = -1;
};

// Texture that can be updated each frame
class D3D12Texture2DCPUWritable
{
public:
	D3D12Texture2DCPUWritable(const uint32_t inWidth, const uint32_t inHeight, const DXGI_FORMAT inFormat, ID3D12GraphicsCommandList* inCommandList, const uint32_t* inData = nullptr);
	inline eastl::shared_ptr<D3D12Texture2D> GetCurrentImage() { return Textures[D3D12Utility::CurrentFrameIndex % D3D12Utility::NumFramesInFlight]; }

public:
	eastl::shared_ptr<D3D12Texture2D> Textures[D3D12Utility::NumFramesInFlight];
};

class D3D12RenderTarget2D
{

public:
	eastl::unique_ptr<D3D12Texture2D> Texture;
	D3D12_CPU_DESCRIPTOR_HANDLE RTV = {};
	D3D12_CPU_DESCRIPTOR_HANDLE UAV = {};

};

class D3D12DepthBuffer
{

public:
	eastl::unique_ptr<D3D12Texture2D> Texture;
	D3D12_CPU_DESCRIPTOR_HANDLE DSV = {};

	//uint64_t ReadOnlyDSVIdx = -1;

	DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;

	eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ArrayDSVs;
	int32_t ArraySize = 1;
};


