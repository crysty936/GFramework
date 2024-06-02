#include "D3D12GraphicsTypes_Internal.h"
#include "D3D12Utility.h"
#include <combaseapi.h>

D3D12Internal_DescriptorHeap::~D3D12Internal_DescriptorHeap()
{
	Heap->Release();
}

void D3D12Internal_DescriptorHeap::Init(bool inShaderVisible, uint32_t inNumPersistent, D3D12_DESCRIPTOR_HEAP_TYPE inHeapType)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = inNumPersistent;
	rtvHeapDesc.Type = inHeapType;
	rtvHeapDesc.Flags = inShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	D3D12Utility::DXAssert(D3D12Globals::Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&Heap)));

	NumPersistentDescriptors = inNumPersistent;
	DescriptorSize = D3D12Globals::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
	GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();
}

D3D12DescHeapAllocationDesc D3D12Internal_DescriptorHeap::AllocatePersistent()
{
	ASSERT((Allocated + 1) <= NumPersistentDescriptors);

	D3D12DescHeapAllocationDesc newAllocation;
	newAllocation.Index = Allocated;
	newAllocation.CPUHandle = CPUStart;
	newAllocation.CPUHandle.ptr += newAllocation.Index * DescriptorSize;

	++Allocated;

	return newAllocation;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Internal_DescriptorHeap::GetGPUHandle(uint32_t inIndex)
{
	uint64_t gpuPtr = GPUStart.ptr + (DescriptorSize * inIndex);

	return D3D12_GPU_DESCRIPTOR_HANDLE{ gpuPtr };
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Internal_DescriptorHeap::GetCPUHandle(uint32_t inIndex)
{
	uint64_t cpuPtr = CPUStart.ptr + (DescriptorSize * inIndex);

	return D3D12_CPU_DESCRIPTOR_HANDLE{ cpuPtr };
}

D3D12Internal_ConstantBuffer::~D3D12Internal_ConstantBuffer()
{
	Handle->Release();
}

void D3D12Internal_ConstantBuffer::Init(const uint64_t inSize)
{
	//D3D12DescHeapAllocationDesc descAlloc = m_CbvSrvHeap.AllocatePersistent();

	// Describe and create a constant buffer view.
	//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	//cbvDesc.SizeInBytes = constantBufferSize;
	//D3D12Globals::Device->CreateConstantBufferView(&cbvDesc, descAlloc.CPUHandle); // Create a descriptor for the Constant Buffer at the given place in the heap

	Size = inSize;

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC constantBufferDesc;
	constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	constantBufferDesc.Alignment = 0;
	constantBufferDesc.Width = inSize * D3D12Globals::NumFramesInFlight;
	constantBufferDesc.Height = 1;
	constantBufferDesc.DepthOrArraySize = 1;
	constantBufferDesc.MipLevels = 1;
	constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantBufferDesc.SampleDesc.Count = 1;
	constantBufferDesc.SampleDesc.Quality = 0;
	constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Handle)));

	GPUAddress = Handle->GetGPUVirtualAddress();

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	D3D12_RANGE readRange = {};
	//readRange.Begin = 0;
	//readRange.End = 0;      // We do not intend to read from this resource on the CPU.

	D3D12Utility::DXAssert(Handle->Map(0, &readRange, reinterpret_cast<void**>(&CPUAddress)));
}

MapResult D3D12Internal_ConstantBuffer::Map()
{
	MapResult res = {};

	const uint64_t offset = (D3D12Globals::CurrentFrameIndex % D3D12Globals::NumFramesInFlight) * Size;
	res.CPUAddress = CPUAddress + offset;
	res.GPUAddress = GPUAddress + offset;

	return res;
}

//D3D12IRenderTargetTexture::D3D12IRenderTargetTexture()
//{
//
//}
//
//D3D12IRenderTargetTexture::~D3D12IRenderTargetTexture()
//{
//	if (Texture.Resource)
//	{
//		Texture.Resource->Release();
//	}
//}


// 
// void D3D12IRenderTargetTexture::Init(const uint32_t inWidth, const uint32_t inHeight)
// {
// 	D3D12_RESOURCE_DESC textureDesc = {};
// 
// 	textureDesc.Width = inWidth;
// 	textureDesc.Height = inHeight;
// 	textureDesc.MipLevels = 1;
// 	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
// 	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
// 	textureDesc.DepthOrArraySize = 1;
// 	textureDesc.SampleDesc.Count = 1;
// 	textureDesc.SampleDesc.Quality = 0;
// 	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
// 	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
// 	textureDesc.Alignment = 0;
// 
// 	const D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
// 
// 	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
// 		&D3D12Utility::GetDefaultHeapProps(),
// 		D3D12_HEAP_FLAG_NONE,
// 		&textureDesc,
// 		initState,
// 		nullptr,
// 		IID_PPV_ARGS(&Texture.Resource)));
// 
// 	static int32_t RenderTargetIndex = 0;
// 	++RenderTargetIndex;
// 
// 	eastl::wstring textureName = L"RenderTarget ";
// 	const eastl::wstring textureIndex = eastl::to_wstring(RenderTargetIndex);
// 	textureName += textureIndex;
// 
// 	Texture.Resource->SetName(textureName.c_str());
// 
// 	// Create SRV
// 	{
// 		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
// 		Texture.SRVIndex = descAllocation.Index;
// 		D3D12Globals::Device->CreateShaderResourceView(Texture.Resource, nullptr, descAllocation.CPUHandle);
// 	}
// 
// 	// Create RTV
// 	{
// 		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalRTVHeap.AllocatePersistent();
// 		RTV = descAllocation.CPUHandle;
// 
// 		D3D12Globals::Device->CreateRenderTargetView(Texture.Resource, nullptr, descAllocation.CPUHandle);
// 
// 	}
// 
// 	Texture.Width = inWidth;
// 	Texture.Height = inHeight;
// 
// 
// 
// 
// }

