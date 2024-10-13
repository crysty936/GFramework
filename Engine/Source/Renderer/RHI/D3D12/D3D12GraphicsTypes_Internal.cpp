#include "D3D12GraphicsTypes_Internal.h"
#include "D3D12Utility.h"
#include <combaseapi.h>
#include "D3D12RHI.h"
#include "Utils/Utils.h"
#include "D3D12Upload.h"

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
	for (uint32_t i = 0; i < NumHeaps; ++i)
	{
		Heaps[i]->Release();
		Heaps[i] = nullptr;
	}
}

void D3D12DescriptorHeap::Init(bool inShaderVisible, uint32_t inNumPersistent, D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32_t inNumTemp)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = inNumPersistent + inNumTemp;
	rtvHeapDesc.Type = inHeapType;
	rtvHeapDesc.Flags = inShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	NumHeaps = inShaderVisible ? D3D12Utility::NumFramesInFlight : 1;

	for (uint32_t i = 0; i < NumHeaps; ++i)
	{
		DXAssert(D3D12Globals::Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&Heaps[i])));
	}

	NumPersistentDescriptors = inNumPersistent;
	NumTempDescriptors = inNumTemp;
	DescriptorSize = D3D12Globals::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	for (uint32_t i = 0; i < NumHeaps; ++i)
	{
		CPUStart[i] = Heaps[i]->GetCPUDescriptorHandleForHeapStart();
		GPUStart[i] = Heaps[i]->GetGPUDescriptorHandleForHeapStart();
	}
}

D3D12DescHeapAllocationDesc D3D12DescriptorHeap::AllocatePersistent()
{
	ASSERT((Allocated + 1) <= NumPersistentDescriptors);

	D3D12DescHeapAllocationDesc newAllocation;
	newAllocation.Index = Allocated;

	for (uint32_t i = 0; i < NumHeaps; ++i)
	{
		newAllocation.CPUHandle[i] = CPUStart[i];
		newAllocation.CPUHandle[i].ptr += newAllocation.Index * DescriptorSize;
	}

	++Allocated;

	return newAllocation;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHandle(uint64_t inIndex, const uint64_t inHeapIdx)
{
	ASSERT(inHeapIdx != uint64_t(-1));

	uint64_t gpuPtr = GPUStart[inHeapIdx].ptr + (DescriptorSize * inIndex);

	return D3D12_GPU_DESCRIPTOR_HANDLE{ gpuPtr };
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUHandle(uint64_t inIndex, const uint64_t inHeapIdx)
{
	uint64_t cpuPtr = CPUStart[inHeapIdx].ptr + (DescriptorSize * inIndex);

	return D3D12_CPU_DESCRIPTOR_HANDLE{ cpuPtr };
}

D3D12DescHeapAllocationDesc D3D12DescriptorHeap::AllocateTemporary(uint32_t inCount)
{
	ASSERT((AllocatedTemp + 1) <= NumTempDescriptors);

	D3D12DescHeapAllocationDesc newAllocation;
	newAllocation.Index = NumPersistentDescriptors + AllocatedTemp;

	newAllocation.CPUHandle[0] = CPUStart[D3D12Utility::CurrentFrameIndex];
	newAllocation.CPUHandle[0].ptr += newAllocation.Index * DescriptorSize;

	AllocatedTemp+= inCount;

	return newAllocation;
}

void D3D12DescriptorHeap::EndFrame()
{
	AllocatedTemp = 0;
}

D3D12ConstantBuffer::~D3D12ConstantBuffer()
{
	Resource->Release();
	Resource = nullptr;
}

void D3D12ConstantBuffer::Init(const uint64_t inSize)
{
	//D3D12DescHeapAllocationDesc descAlloc = m_CbvSrvHeap.AllocatePersistent();

	// Describe and create a constant buffer view.
	//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	//cbvDesc.SizeInBytes = constantBufferSize;
	//D3D12Globals::Device->CreateConstantBufferView(&cbvDesc, descAlloc.CPUHandle); // Create a descriptor for the Constant Buffer at the given place in the heap

	Size = Utils::AlignTo(inSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC constantBufferDesc;
	constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	constantBufferDesc.Alignment = 0;
	constantBufferDesc.Width = Size * D3D12Utility::NumFramesInFlight;
	constantBufferDesc.Height = 1;
	constantBufferDesc.DepthOrArraySize = 1;
	constantBufferDesc.MipLevels = 1;
	constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantBufferDesc.SampleDesc.Count = 1;
	constantBufferDesc.SampleDesc.Quality = 0;
	constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Resource)));

	GPUAddress = Resource->GetGPUVirtualAddress();

	static uint64_t ConstantBufferIdx = 0;
	++ConstantBufferIdx;

	eastl::wstring bufferName = L"ConstantBuffer_";
	bufferName.append(eastl::to_wstring(ConstantBufferIdx));

	Resource->SetName(bufferName.c_str());

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	D3D12_RANGE readRange = {};
	//readRange.Begin = 0;
	//readRange.End = 0;      // We do not intend to read from this resource on the CPU.

	DXAssert(Resource->Map(0, &readRange, reinterpret_cast<void**>(&CPUAddress)));
}

MapResult D3D12ConstantBuffer::Map()
{
	MapResult res = {};

	const uint64_t offset = (D3D12Utility::CurrentFrameIndex % D3D12Utility::NumFramesInFlight) * Size;
	res.CPUAddress = CPUAddress + offset;
	res.GPUAddress = GPUAddress + offset;

	return res;
}

MapResult D3D12ConstantBuffer::ReserveTempBufferMemory(const uint64_t inSize)
{
	uint64_t& currentMemoryCounter = UsedMemory[D3D12Utility::CurrentFrameIndex];
	const uint64_t alignedSize = Utils::AlignTo(inSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	ASSERT_MSG(alignedSize < (Size - currentMemoryCounter), "More memory required than available, increase buffer size.");


	// Use current memory counter position as offset from current buffer start
	const uint64_t offset = currentMemoryCounter;

	MapResult currentBufferStartMap = Map();
	uint8_t* CPUAddress = currentBufferStartMap.CPUAddress;
	uint64_t GPUAddress = currentBufferStartMap.GPUAddress;

	CPUAddress += offset;
	GPUAddress += offset;

	// Add this to the used memory
	currentMemoryCounter += alignedSize;

	return { CPUAddress, GPUAddress };
}

void D3D12ConstantBuffer::ClearUsedMemory()
{
	UsedMemory[D3D12Utility::CurrentFrameIndex] = 0;
}

D3D12StructuredBuffer::~D3D12StructuredBuffer()
{
	Resource->Release();
	Resource = nullptr;
}

void D3D12StructuredBuffer::Init(const uint64_t inNumElements, const uint64_t inStride)
{
	NumElements = inNumElements;
	Stride = inStride;

	const uint64_t totalSize = inNumElements * inStride;
	Size = Utils::AlignTo(totalSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	D3D12_HEAP_PROPERTIES heapProps = D3D12Utility::GetDefaultHeapProps();

	D3D12_RESOURCE_DESC constantBufferDesc;
	constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	constantBufferDesc.Alignment = 0;
	constantBufferDesc.Width = Size * D3D12Utility::NumFramesInFlight;
	constantBufferDesc.Height = 1;
	constantBufferDesc.DepthOrArraySize = 1;
	constantBufferDesc.MipLevels = 1;
	constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantBufferDesc.SampleDesc.Count = 1;
	constantBufferDesc.SampleDesc.Quality = 0;
	constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&Resource)));

	GPUAddress = Resource->GetGPUVirtualAddress();

	static uint64_t StructuredBufferIdx = 0;
	eastl::wstring bufferName = L"StructuredBuffer_";
	bufferName.append(eastl::to_wstring(StructuredBufferIdx));
	++StructuredBufferIdx;

	Resource->SetName(bufferName.c_str());

}

void D3D12StructuredBuffer::UploadDataAllFrames(void* inData, uint64_t inSize)
{
	for (uint32_t i = 0; i < D3D12Utility::NumFramesInFlight; ++i)
	{
		const uint64_t offset = (i % D3D12Utility::NumFramesInFlight) * Size;
		UploadContext bufferUploadContext = D3D12Upload::ResourceUploadBegin(inSize);

		memcpy(bufferUploadContext.CPUAddress, inData, inSize);
		bufferUploadContext.CmdList->CopyBufferRegion(Resource, offset, bufferUploadContext.Resource, bufferUploadContext.ResourceOffset, inSize);

		D3D12Upload::ResourceUploadEnd(bufferUploadContext);
	}
}

void D3D12StructuredBuffer::UploadDataCurrentFrame(void* inData, uint64_t inSize)
{
	const uint64_t offset = (D3D12Utility::CurrentFrameIndex % D3D12Utility::NumFramesInFlight) * Size;
	UploadContext bufferUploadContext = D3D12Upload::ResourceUploadBegin(inSize);

	memcpy(bufferUploadContext.CPUAddress, inData, inSize);
	bufferUploadContext.CmdList->CopyBufferRegion(Resource, offset, bufferUploadContext.Resource, bufferUploadContext.ResourceOffset, inSize);

	D3D12Upload::ResourceUploadEnd(bufferUploadContext);
}

uint64_t D3D12StructuredBuffer::GetCurrentGPUAddress()
{
	return GPUAddress + ((D3D12Utility::CurrentFrameIndex % D3D12Utility::NumFramesInFlight) * Size);
}

D3D12RawBuffer::~D3D12RawBuffer()
{
	Resource->Release();
	Resource = nullptr;
}

void D3D12RawBuffer::Init(const uint64_t inNumElements)
{
	NumElements = inNumElements;

	const uint64_t totalSize = inNumElements * sizeof(uint32_t);
	Size = Utils::AlignTo(totalSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	// Right now, it's supposed to only be written to by compute
	D3D12_HEAP_PROPERTIES heapProps = D3D12Utility::GetDefaultHeapProps();

	D3D12_RESOURCE_DESC constantBufferDesc;
	constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	constantBufferDesc.Alignment = 0;
	// Not cpu visible so not double buffered
	constantBufferDesc.Width = Size;
	constantBufferDesc.Height = 1;
	constantBufferDesc.DepthOrArraySize = 1;
	constantBufferDesc.MipLevels = 1;
	constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantBufferDesc.SampleDesc.Count = 1;
	constantBufferDesc.SampleDesc.Quality = 0;
	constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Resource)));

	GPUAddress = Resource->GetGPUVirtualAddress();

	static uint64_t RawBuffer = 0;
	eastl::wstring bufferName = L"RawBuffer_";
	bufferName.append(eastl::to_wstring(RawBuffer));
	++RawBuffer;


	// Create UAV
	{
		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalUAVHeap.AllocatePersistent();
		UAV = descAllocation.CPUHandle[0];

		D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.Buffer.CounterOffsetInBytes = 0;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		desc.Buffer.NumElements = uint32_t(NumElements);

		D3D12Globals::Device->CreateUnorderedAccessView(Resource, nullptr, &desc, UAV);
	}


	Resource->SetName(bufferName.c_str());
}

uint64_t D3D12RawBuffer::GetGPUAddress()
{
	return GPUAddress;
}

D3D12Fence::~D3D12Fence()
{
	FenceHandle->Release();
	CloseHandle(FenceEvent);
}

void D3D12Fence::Init(uint64_t inInitialValue)
{
	DXAssert(D3D12Globals::Device->CreateFence(inInitialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FenceHandle)));

	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		DXAssert(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void D3D12Fence::Signal(ID3D12CommandQueue* inQueue, uint64_t inValue)
{
	DXAssert(inQueue->Signal(FenceHandle, inValue));
}

void D3D12Fence::Wait(uint64_t inValue)
{
	if (!IsCompleted(inValue))
	{
		// Tell fence to raise this event once it's equal to fence value
		DXAssert(FenceHandle->SetEventOnCompletion(inValue, FenceEvent));

		// Wait until that event is raised
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

bool D3D12Fence::IsCompleted(uint64_t inValue) const
{
	return GetValue() >= inValue;
}

uint64_t D3D12Fence::GetValue() const
{
	const uint64_t fenceValue = FenceHandle->GetCompletedValue();

	// Happens if Graphics device is removed while running
	ASSERT(fenceValue != UINT64_MAX);

	return fenceValue;
}

