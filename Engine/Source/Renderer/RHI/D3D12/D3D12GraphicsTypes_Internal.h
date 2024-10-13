#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "D3D12Resources.h"
#include "D3D12Utility.h"

struct D3D12DescHeapAllocationDesc
{
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle[D3D12Utility::NumFramesInFlight] = {};
	uint32_t Index = -1;
};

struct D3D12DescriptorHeap
{
public:
	~D3D12DescriptorHeap();

	void Init(bool inShaderVisible, uint32_t inNumPersistent, D3D12_DESCRIPTOR_HEAP_TYPE inHeapType, uint32_t inNumTemp = 0);
	D3D12DescHeapAllocationDesc AllocatePersistent();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint64_t inIndex, const uint64_t inHeapIdx);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint64_t inIndex, const uint64_t inHeapIdx);

	D3D12DescHeapAllocationDesc AllocateTemporary(uint32_t inCount);

	void EndFrame();

public:
	ID3D12DescriptorHeap* Heaps[D3D12Utility::NumFramesInFlight] = {};
	uint32_t NumPersistentDescriptors = 0;
	uint32_t NumTempDescriptors = 0;
	uint32_t NumHeaps = 0;

	uint32_t DescriptorSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUStart[D3D12Utility::NumFramesInFlight] = {};
	D3D12_GPU_DESCRIPTOR_HANDLE GPUStart[D3D12Utility::NumFramesInFlight] = {};

private:
	uint32_t Allocated = 0;
	uint32_t AllocatedTemp = 0;
};

struct MapResult
{
	uint8_t* CPUAddress = nullptr;
	uint64_t GPUAddress = 0;
};

struct D3D12ConstantBuffer
{

public:
	~D3D12ConstantBuffer();

	void Init(const uint64_t inSize);

	MapResult Map();

	inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() { return D3D12_GPU_DESCRIPTOR_HANDLE{ GPUAddress }; }

	MapResult ReserveTempBufferMemory(const uint64_t inSize);

	// Should be called at the start of each frime
	void ClearUsedMemory();

public:

	ID3D12Resource* Resource = nullptr;
	uint8_t* CPUAddress = nullptr;
	uint64_t GPUAddress = 0;
	uint64_t Size = 0;
	uint64_t UsedMemory[D3D12Utility::NumFramesInFlight] = {0, 0};
};

class D3D12StructuredBuffer
{
public:
	~D3D12StructuredBuffer();

	void Init(const uint64_t inNumElements, const uint64_t inStride);
	void UploadDataAllFrames(void* inData, uint64_t inSize);
	void UploadDataCurrentFrame(void* inData, uint64_t inSize);
	uint64_t GetCurrentGPUAddress();

public:
	struct ID3D12Resource* Resource = nullptr;
	uint64_t NumElements = 0;
	uint64_t Stride = 0;
	uint64_t Size = 0;

	uint64_t GPUAddress = 0;
};

class D3D12RawBuffer
{
public:
	~D3D12RawBuffer();

	void Init(const uint64_t inNumElements);
	uint64_t GetGPUAddress();

public:
	struct ID3D12Resource* Resource = nullptr;
	uint64_t NumElements = 0;
	uint64_t Size = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE UAV = {};

	uint64_t GPUAddress = 0;
};

struct D3D12Fence
{
	~D3D12Fence();

	ID3D12Fence* FenceHandle = nullptr;
	HANDLE FenceEvent = INVALID_HANDLE_VALUE;

	void Init(uint64_t inInitialValue = 0);

	void Signal(ID3D12CommandQueue* inQueue, uint64_t inValue);
	void Wait(uint64_t inValue);
	bool IsCompleted(uint64_t inValue) const;
	uint64_t GetValue() const;



};


