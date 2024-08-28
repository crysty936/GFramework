#pragma once
#include "Renderer/RHI/RHI.h"
#include "EASTL/string.h"
#include <winerror.h>
#include "Core/EngineUtils.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include "D3D12GraphicsTypes_Internal.h"

enum class ERasterizerState : uint8_t
{
	BackFaceCull,
	FrontFaceCull,
	Count
};

enum class EBlendState : uint8_t
{
	Disabled,
	Count
};

namespace D3D12Globals
{
	extern ID3D12Device* Device;
	extern IDXGISwapChain3* SwapChain;
	extern ID3D12CommandQueue* GraphicsCommandQueue;

	extern uint64_t CurrentFrameIndex;
	constexpr uint32_t NumFramesInFlight = 2;

	// Descriptor Heaps
	// TODO: Implement non-shader visible descriptor heaps that will be copied over into main heap when drawing
	extern D3D12DescriptorHeap GlobalRTVHeap;
	extern D3D12DescriptorHeap GlobalSRVHeap;
}

inline bool DXAssert(HRESULT inRez)
{
	const bool success = SUCCEEDED(inRez);
	ASSERT_MSG(success, "Direct3D12 Operation failed with code 0x%08X", static_cast<uint32_t>(inRez));

	return success;
}

namespace D3D12Utility
{
	void Init();

	D3D12_HEAP_PROPERTIES& GetDefaultHeapProps();
	D3D12_HEAP_PROPERTIES& GetUploadHeapProps();

	void TransitionResource(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource, D3D12_RESOURCE_STATES inStateBefore, D3D12_RESOURCE_STATES inStateAfter);

	void MakeTextureReadable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);
	void MakeTextureWriteable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);

	D3D12_RASTERIZER_DESC GetRasterizerState(const ERasterizerState inForState);
	D3D12_BLEND_DESC GetBlendState(const EBlendState inForState);

	constexpr float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
}