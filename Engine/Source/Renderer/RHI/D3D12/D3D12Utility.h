#pragma once
#include "Renderer/RHI/RHI.h"
#include "EASTL/string.h"
#include <winerror.h>
#include "Core/EngineUtils.h"
#include <d3d12.h>
#include "D3D12GraphicsTypes_Internal.h"

namespace D3D12Globals
{
	extern ID3D12Device* Device;
	extern uint64_t CurrentFrameIndex;
	constexpr uint32_t NumFramesInFlight = 2;

	// Descriptor Heaps
	// TODO: Implement non-shader visible descriptor heaps that will be copied over into main heap when drawing
	extern D3D12Internal_DescriptorHeap GlobalRTVHeap;
	extern D3D12Internal_DescriptorHeap GlobalSRVHeap;
}

namespace D3D12Utility
{
	inline bool DXAssert(HRESULT inRez)
	{
		const bool success = SUCCEEDED(inRez);
		ASSERT_MSG(success, "Direct3D12 Operation failed with code 0x%08X", static_cast<uint32_t>(inRez));

		return success;
	}

	D3D12_HEAP_PROPERTIES& GetDefaultHeapProps();

	void TransitionResource(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource, D3D12_RESOURCE_STATES inStateBefore, D3D12_RESOURCE_STATES inStateAfter);

	void MakeTextureReadable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);
	void MakeTextureWriteable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource);


}