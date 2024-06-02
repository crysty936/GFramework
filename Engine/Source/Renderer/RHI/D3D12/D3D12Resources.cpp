#include "D3D12Resources.h"
#include "D3D12Utility.h"
#include "D3D12RHI.h"

// Exclude rarely-used stuff from Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             
#endif

#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;


eastl::shared_ptr<class D3D12IndexBuffer> D3D12RHI::CreateIndexBuffer(const uint32_t* inData, uint32_t inCount)
{
	ID3D12Resource* resource = nullptr;
	const UINT indexBufferSize = sizeof(uint32_t) * inCount;

	D3D12_RESOURCE_DESC indexBufferDesc;
	indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	indexBufferDesc.Alignment = 0;
	indexBufferDesc.Width = indexBufferSize;
	indexBufferDesc.Height = 1;
	indexBufferDesc.DepthOrArraySize = 1;
	indexBufferDesc.MipLevels = 1;
	indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	indexBufferDesc.SampleDesc.Count = 1;
	indexBufferDesc.SampleDesc.Quality = 0;
	indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	indexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// TODO:
	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Replace this with default heap.
	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&indexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)));


	// Copy the triangle data to the index buffer.
	UINT8* pIndexDataBegin;
	D3D12_RANGE indexReadRange;
	indexReadRange.Begin = 0;
	indexReadRange.End = 0;
	D3D12Utility::DXAssert(resource->Map(0, &indexReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));

	memcpy(pIndexDataBegin, inData, indexBufferSize);

	resource->Unmap(0, nullptr);

	eastl::shared_ptr<D3D12IndexBuffer> newBuffer = eastl::make_shared<D3D12IndexBuffer>();
	newBuffer->IndexCount = inCount;
	newBuffer->Resource = resource;
	newBuffer->IBFormat = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
	newBuffer->GPUAddress = resource->GetGPUVirtualAddress();;
	newBuffer->Size = indexBufferSize;

	return newBuffer;
}

eastl::shared_ptr<class D3D12VertexBuffer> D3D12RHI::CreateVertexBuffer(const class VertexInputLayout& inLayout, const float* inVertices, const int32_t inCount, eastl::shared_ptr<class D3D12IndexBuffer> inIndexBuffer /*= nullptr*/)
{
	ID3D12Resource* resource = nullptr;

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	const UINT vertexBufferSize = sizeof(float) * inCount;

	D3D12_RESOURCE_DESC vertexBufferDesc;
	vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferDesc.Alignment = 0;
	vertexBufferDesc.Width = vertexBufferSize;
	vertexBufferDesc.Height = 1;
	vertexBufferDesc.DepthOrArraySize = 1;
	vertexBufferDesc.MipLevels = 1;
	vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexBufferDesc.SampleDesc.Count = 1;
	vertexBufferDesc.SampleDesc.Quality = 0;
	vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)));

	// Copy the vertex data to the vertex buffer.
	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = 0;
	D3D12Utility::DXAssert(resource->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));

	memcpy(pVertexDataBegin, inVertices, vertexBufferSize);
	resource->Unmap(0, nullptr);

	eastl::shared_ptr<D3D12VertexBuffer> newBuffer = eastl::make_shared<D3D12VertexBuffer>();
	newBuffer->GPUAddress = resource->GetGPUVirtualAddress();
	newBuffer->AllocatedSize = vertexBufferSize;
	newBuffer->Resource = resource;
	newBuffer->Layout = inLayout;
	newBuffer->NumElements = inCount;

	return newBuffer;
}

D3D12IndexBuffer::D3D12IndexBuffer()
	: RHIIndexBuffer()
{}

D3D12_INDEX_BUFFER_VIEW D3D12IndexBuffer::IBView() const
{
	D3D12_INDEX_BUFFER_VIEW ibView;
	ibView.BufferLocation = GPUAddress;
	ibView.Format = IBFormat;
	ibView.SizeInBytes = Size;

	return ibView;
}

D3D12VertexBuffer::D3D12VertexBuffer()
	: RHIVertexBuffer()
{}

D3D12_VERTEX_BUFFER_VIEW D3D12VertexBuffer::VBView() const
{
	D3D12_VERTEX_BUFFER_VIEW outView = {};

	outView.BufferLocation = GPUAddress;
	outView.StrideInBytes = Layout.GetStride();
	//outView.StrideInBytes = 3;// 8 floats
	outView.SizeInBytes = (uint32_t)AllocatedSize;

	return outView;
}
