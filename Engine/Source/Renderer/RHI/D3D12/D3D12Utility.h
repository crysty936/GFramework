#pragma once
#include "Renderer/RHI/RHITypes.h"
#include "EASTL/string.h"
#include <winerror.h>
#include "Core/EngineUtils.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include "WinPixEventRuntime/pix3.h"
#include "Renderer/RHI/Resources/RHITexture.h"

struct PIXMarker
{
	ID3D12GraphicsCommandList* CmdList = nullptr;

	PIXMarker(ID3D12GraphicsCommandList* inCmdList, const char* inMarkerName)
		:CmdList(inCmdList)
	{
		PIXBeginEvent(CmdList, 0, inMarkerName);
	}

	~PIXMarker()
	{
		PIXEndEvent(CmdList);
	}
};

inline D3D12_RESOURCE_STATES TexStateToD3D12ResState(const ETextureState inState)
{
	D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;

	switch (inState)
	{
	case ETextureState::Present:
		initState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
		break;
	case ETextureState::Shader_Resource:
		initState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		break;
	case ETextureState::Render_Target:
		initState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		break;
	case ETextureState::Depth_Write:
		initState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		break;
	default:
		break;
	}

	return initState;
}

inline bool DXAssert(HRESULT inRez)
{
	const bool success = SUCCEEDED(inRez);
	//ASSERT_MSG(success, "Direct3D12 Operation failed with code 0x%08X", static_cast<uint32_t>(inRez));

	return success;
}

namespace D3D12Utility
{
	void Init();

	extern uint64_t CurrentFrameIndex;
	constexpr uint32_t NumFramesInFlight = 2;

	D3D12_HEAP_PROPERTIES& GetDefaultHeapProps();
	D3D12_HEAP_PROPERTIES& GetUploadHeapProps();

	void UAVBarrier(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);
	void TransitionResource(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource, D3D12_RESOURCE_STATES inStateBefore, D3D12_RESOURCE_STATES inStateAfter);
	D3D12_GPU_DESCRIPTOR_HANDLE CreateTempDescriptorTable(ID3D12GraphicsCommandList* inCmdList, const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& inHandles);
	void BindTempDescriptorTable(uint32_t inRootParamIdx, ID3D12GraphicsCommandList* inCmdList, const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& inHandles);

	void MakeTextureReadable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);
	void MakeTextureWriteable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);

	D3D12_RASTERIZER_DESC GetRasterizerState(const ERasterizerState inForState);
	D3D12_BLEND_DESC GetBlendState(const EBlendState inForState);
	D3D12_DEPTH_STENCIL_DESC GetDepthState(const EDepthState inForState);

	constexpr float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
}