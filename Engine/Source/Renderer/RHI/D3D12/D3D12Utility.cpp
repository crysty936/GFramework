#include "D3D12Utility.h"

ID3D12Device* D3D12Globals::Device;
uint64_t D3D12Globals::CurrentFrameIndex = 0;
D3D12Internal_DescriptorHeap D3D12Globals::GlobalRTVHeap;
D3D12Internal_DescriptorHeap D3D12Globals::GlobalSRVHeap;

D3D12_HEAP_PROPERTIES& D3D12Utility::GetDefaultHeapProps()
{
	static D3D12_HEAP_PROPERTIES DefaultHeapProps
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

// 	DefaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
// 	DefaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
// 	DefaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
// 	DefaultHeapProps.CreationNodeMask = 1;
// 	DefaultHeapProps.VisibleNodeMask = 1;

	return DefaultHeapProps;
}


D3D12_RESOURCE_BARRIER MakeTransitionBarrier(ID3D12Resource* inResource, D3D12_RESOURCE_STATES inStateBefore, D3D12_RESOURCE_STATES inStateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = inResource;
	barrier.Transition.StateBefore = inStateBefore;
	barrier.Transition.StateAfter = inStateAfter;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	return barrier;
}

void D3D12Utility::TransitionResource(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource, D3D12_RESOURCE_STATES inStateBefore, D3D12_RESOURCE_STATES inStateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = MakeTransitionBarrier(inResource, inStateBefore, inStateAfter);

	inCmdList->ResourceBarrier(1, &barrier);
}

void D3D12Utility::MakeTextureReadable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource)
{
	TransitionResource(inCmdList, inResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void D3D12Utility::MakeTextureWriteable(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource)
{
	TransitionResource(inCmdList, inResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

