#include "D3D12Utility.h"
#include "D3D12GraphicsTypes_Internal.h"
#include "D3D12RHI.h"

uint64_t D3D12Utility::CurrentFrameIndex = 0;

D3D12_RASTERIZER_DESC RasterizerStates[ERasterizerState::Count];
D3D12_BLEND_DESC BlendStates[EBlendState::Count];
D3D12_DEPTH_STENCIL_DESC DepthStates[EDepthState::Count];

void D3D12Utility::Init()
{
	// Rasterirez States
	{
		{
			D3D12_RASTERIZER_DESC& cullDisabledDesc = RasterizerStates[uint8_t(ERasterizerState::Disabled)];

			cullDisabledDesc.FillMode = D3D12_FILL_MODE_SOLID;
			cullDisabledDesc.CullMode = D3D12_CULL_MODE_NONE;
			cullDisabledDesc.FrontCounterClockwise = false;
			cullDisabledDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			cullDisabledDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			cullDisabledDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			cullDisabledDesc.DepthClipEnable = true;
			cullDisabledDesc.MultisampleEnable = false;
			cullDisabledDesc.AntialiasedLineEnable = false;
			cullDisabledDesc.ForcedSampleCount = 0;
			cullDisabledDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

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

	// Depth States
	{
		// Disabled
		{
			D3D12_DEPTH_STENCIL_DESC& desc = DepthStates[uint8_t(EDepthState::Disabled)];
			desc.DepthEnable = false;
			desc.StencilEnable = false;
			desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		}

		// Enabled
		{
			D3D12_DEPTH_STENCIL_DESC& desc = DepthStates[uint8_t(EDepthState::WriteEnabled)];
			desc.DepthEnable = true;
			desc.StencilEnable = FALSE;
			desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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

void D3D12Utility::UAVBarrier(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = inResource;

	inCmdList->ResourceBarrier(1, &barrier);
}

void D3D12Utility::TransitionResource(ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* inResource, D3D12_RESOURCE_STATES inStateBefore, D3D12_RESOURCE_STATES inStateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = MakeTransitionBarrier(inResource, inStateBefore, inStateAfter);

	inCmdList->ResourceBarrier(1, &barrier);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Utility::CreateTempDescriptorTable(ID3D12GraphicsCommandList* inCmdList, const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& inHandles)
{
	const D3D12DescHeapAllocationDesc tempDesc = D3D12Globals::GlobalSRVHeap.AllocateTemporary(inHandles.size());

	const uint32_t destRanges[1] = { inHandles.size() };
	D3D12Globals::Device->CopyDescriptorsSimple(inHandles.size(), tempDesc.CPUHandle[0], inHandles[0], D3D12Globals::GlobalSRVHeap.HeapType);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { D3D12Globals::GlobalSRVHeap.GetGPUHandle(tempDesc.Index, D3D12Utility::CurrentFrameIndex) };

	return gpuHandle;
}

void D3D12Utility::BindTempDescriptorTable(uint32_t inRootParamIdx, ID3D12GraphicsCommandList* inCmdList, const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& inHandles)
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = CreateTempDescriptorTable(inCmdList, inHandles);

	inCmdList->SetComputeRootDescriptorTable(inRootParamIdx, gpuHandle);
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

D3D12_DEPTH_STENCIL_DESC D3D12Utility::GetDepthState(const EDepthState inForState)
{
	return DepthStates[uint8_t(inForState)];
}

