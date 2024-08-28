#include "D3D12Utility.h"

ID3D12Device* D3D12Globals::Device;
IDXGISwapChain3* D3D12Globals::SwapChain;
ID3D12CommandQueue* D3D12Globals::GraphicsCommandQueue;
uint64_t D3D12Globals::CurrentFrameIndex = 0;
D3D12DescriptorHeap D3D12Globals::GlobalRTVHeap;
D3D12DescriptorHeap D3D12Globals::GlobalSRVHeap;

D3D12_RASTERIZER_DESC RasterizerStates[ERasterizerState::Count];
D3D12_BLEND_DESC BlendStates[EBlendState::Count];

void D3D12Utility::Init()
{
	// Rasterirez States
	{
		{
			D3D12_RASTERIZER_DESC& cullBackFaceDesc = RasterizerStates[uint8_t(ERasterizerState::BackFaceCull)];

			cullBackFaceDesc.FillMode = D3D12_FILL_MODE_SOLID;
			cullBackFaceDesc.CullMode = D3D12_CULL_MODE_BACK;
			//cullBackFaceDesc.CullMode = D3D12_CULL_MODE_FRONT;
			cullBackFaceDesc.FrontCounterClockwise = false;
			//cullBackFaceDesc.FrontCounterClockwise = true;
			cullBackFaceDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			cullBackFaceDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			cullBackFaceDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			cullBackFaceDesc.DepthClipEnable = true;
			cullBackFaceDesc.MultisampleEnable = false;
			cullBackFaceDesc.AntialiasedLineEnable = false;
			cullBackFaceDesc.ForcedSampleCount = 0;
			cullBackFaceDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

		{
			D3D12_RASTERIZER_DESC& cullFrontFaceDesc = RasterizerStates[uint8_t(ERasterizerState::FrontFaceCull)];

			cullFrontFaceDesc.FillMode = D3D12_FILL_MODE_SOLID;
			cullFrontFaceDesc.CullMode = D3D12_CULL_MODE_FRONT;
			cullFrontFaceDesc.FrontCounterClockwise = false;
			//cullFrontFaceDesc.FrontCounterClockwise = true;
			cullFrontFaceDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			cullFrontFaceDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			cullFrontFaceDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			cullFrontFaceDesc.DepthClipEnable = true;
			cullFrontFaceDesc.MultisampleEnable = false;
			cullFrontFaceDesc.AntialiasedLineEnable = false;
			cullFrontFaceDesc.ForcedSampleCount = 0;
			cullFrontFaceDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}



	}

	// Blend States
	{
		D3D12_BLEND_DESC& disabledBlendDesc = BlendStates[uint8_t(EBlendState::Disabled)];

		disabledBlendDesc.AlphaToCoverageEnable = FALSE;
		disabledBlendDesc.IndependentBlendEnable = FALSE;

		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};

		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			disabledBlendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
		}
	}

}

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


D3D12_HEAP_PROPERTIES& D3D12Utility::GetUploadHeapProps()
{
	static D3D12_HEAP_PROPERTIES UploadHeapProps
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	return UploadHeapProps;
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


D3D12_RASTERIZER_DESC D3D12Utility::GetRasterizerState(const ERasterizerState inForState)
{
	return RasterizerStates[uint8_t(inForState)];
}

D3D12_BLEND_DESC D3D12Utility::GetBlendState(const EBlendState inForState)
{
	return BlendStates[uint8_t(inForState)];
}

