#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "D3D12Resources.h"

struct D3D12DescHeapAllocationDesc
{
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = {};
	uint32_t Index = -1;
};

struct D3D12DescriptorHeap
{
public:
	~D3D12DescriptorHeap();

	void Init(bool inShaderVisible, uint32_t inNumPersistent, D3D12_DESCRIPTOR_HEAP_TYPE inHeapType);
	D3D12DescHeapAllocationDesc AllocatePersistent();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint64_t inIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint64_t inIndex);

public:
	ID3D12DescriptorHeap* Heap = nullptr;
	uint32_t NumPersistentDescriptors = 0;

	uint32_t DescriptorSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUStart = {};
	D3D12_GPU_DESCRIPTOR_HANDLE GPUStart = {};

private:
	uint32_t Allocated = 0;
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

public:

	ID3D12Resource* D3D12Resource = nullptr;
	uint8_t* CPUAddress = nullptr;
	uint64_t GPUAddress = 0;
	uint64_t Size = 0;


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

//struct D3D12IRenderTargetTexture
//{
//public:
//	//D3D12IRenderTargetTexture();
//	//~D3D12IRenderTargetTexture();
//
//	//void Init(const uint32_t inWidth, const uint32_t inHeight);
//
//public:
//	//D3D12Texture2D Texture;
//
//	D3D12_CPU_DESCRIPTOR_HANDLE RTV = {};
//
//
//
//
//};
//
//
//



