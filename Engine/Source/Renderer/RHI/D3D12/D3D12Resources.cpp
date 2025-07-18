#include "D3D12Resources.h"
#include "D3D12Utility.h"
#include "D3D12RHI.h"
#include "DirectXTex.h"
#include "Utils/ImageLoading.h"
#include "D3D12Upload.h"

//#include "glm/ext/vector_common.inl"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"


// Exclude rarely-used stuff from Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             
#endif

#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgidebug.h>
#include "Core/WindowsPlatform.h"

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

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&D3D12Utility::GetDefaultHeapProps(),
		D3D12_HEAP_FLAG_NONE,
		&indexBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&resource)));

	static uint64_t IndexBufferIdx = 0;
	++IndexBufferIdx;
	eastl::wstring bufferName = L"IndexBuffer_";
	bufferName.append(eastl::to_wstring(IndexBufferIdx));
	resource->SetName(bufferName.c_str());

	UploadContext indexUploadContext = D3D12Upload::ResourceUploadBegin(indexBufferSize);

	memcpy(indexUploadContext.CPUAddress, inData, indexBufferSize);
	indexUploadContext.CmdList->CopyBufferRegion(resource, 0, indexUploadContext.Resource, indexUploadContext.ResourceOffset, indexBufferSize);
	D3D12Upload::ResourceUploadEnd(indexUploadContext);

	eastl::shared_ptr<D3D12IndexBuffer> newBuffer = eastl::make_shared<D3D12IndexBuffer>();
	newBuffer->IndexCount = inCount;
	newBuffer->Resource = resource;
	newBuffer->IBFormat = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
	newBuffer->GPUAddress = resource->GetGPUVirtualAddress();;
	newBuffer->Size = indexBufferSize;

	return newBuffer;
}

eastl::shared_ptr<class D3D12VertexBuffer> D3D12RHI::CreateVertexBuffer(const VertexInputLayout& inLayout, const float* inVertices, const int32_t inCount, eastl::shared_ptr<class D3D12IndexBuffer> inIndexBuffer /*= nullptr*/)
{
	ID3D12Resource* resource = nullptr;

	const uint64_t vertexBufferSize = inLayout.Stride * inCount;

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

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&D3D12Utility::GetDefaultHeapProps(),
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&resource)));

	static uint64_t VertexBufferIdx = 0;
	++VertexBufferIdx;
	eastl::wstring bufferName = L"VertexBuffer_";
	bufferName.append(eastl::to_wstring(VertexBufferIdx));
	resource->SetName(bufferName.c_str());

	ASSERT(resource != nullptr);

	UploadContext vertexUploadContext = D3D12Upload::ResourceUploadBegin(vertexBufferDesc.Width);

	memcpy(vertexUploadContext.CPUAddress, inVertices, vertexBufferSize);
	vertexUploadContext.CmdList->CopyBufferRegion(resource, 0, vertexUploadContext.Resource, vertexUploadContext.ResourceOffset, vertexBufferSize);
	D3D12Upload::ResourceUploadEnd(vertexUploadContext);

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
{
}

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
{
}

D3D12_VERTEX_BUFFER_VIEW D3D12VertexBuffer::VBView() const
{
	D3D12_VERTEX_BUFFER_VIEW outView = {};

	outView.BufferLocation = GPUAddress;
	outView.StrideInBytes = Layout.GetStride();
	//outView.StrideInBytes = 3;// 8 floats
	outView.SizeInBytes = (uint32_t)AllocatedSize;

	return outView;
}

void UploadTexture(ID3D12Resource* inDestResource, const uint32_t inWidth, const uint32_t inHeight, const DXGI_FORMAT inTexFormat, const int32_t inNumMips, const void* inData, const UploadContext& inContext, const bool bIsCubemap = false)
{
	const uint32_t numMips = (uint32_t)inNumMips;
	const uint64_t arraySize = bIsCubemap ? 6u : 1u; // Use texture array size when creating texture arrays

	uint32_t NumSubresources = arraySize * numMips;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * NumSubresources));
	ASSERT(layouts != nullptr);

	const D3D12_RESOURCE_DESC DestDesc = inDestResource->GetDesc();

	uint64_t requiredSize = 0;
	uint64_t* rowSize = (uint64_t*)(_alloca(sizeof(uint64_t) * NumSubresources));
	uint32_t* numRows = (uint32_t*)(_alloca(sizeof(uint32_t) * NumSubresources));
	D3D12Globals::Device->GetCopyableFootprints(&DestDesc, 0, NumSubresources, 0, layouts, numRows, rowSize, &requiredSize);

	ID3D12Resource* uploadBuffer = inContext.Resource;
	const D3D12_RESOURCE_DESC uploadBufferDesc = uploadBuffer->GetDesc();

	const bool bIsRightDimensionFormat = uploadBufferDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
	const bool bIsSmallerThanWidth = uploadBufferDesc.Width >= requiredSize + layouts[0].Offset;
	const bool bIsValidSize = requiredSize <= size_t(-1);
	const bool bIsDestRightFormat = (DestDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || (NumSubresources == 1));

	const bool isCopyValid = bIsRightDimensionFormat && bIsSmallerThanWidth & bIsValidSize && bIsDestRightFormat;

	ASSERT(isCopyValid);

	uint8_t* uploadMem = reinterpret_cast<uint8_t*>(inContext.CPUAddress);
	const uint8_t* srcMem = reinterpret_cast<const uint8_t*>(inData);
	const uint64_t srcTexelSize = DirectX::BitsPerPixel(inTexFormat) / 8;

	// Copy to the staging upload heap
	// Go through all array elements, eg. different textures in an array or different slices of a cubemap
	for (uint32_t arrayIdx = 0; arrayIdx < arraySize; ++arrayIdx)
	{
		uint64_t mipWidth = inWidth;

		// Each array element has its own mips
		// Each mip is a subresource
		for (uint64_t mipIdx = 0; mipIdx < numMips; ++mipIdx)
		{
			ASSERT(rowSize[mipIdx] <= size_t(-1));

			const uint64_t subResourceIdx = mipIdx + (arrayIdx * numMips);
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subresourceLayout = layouts[subResourceIdx];
			const uint64_t subResourceHeight = numRows[subResourceIdx];
			const uint64_t subResourcePitch = subresourceLayout.Footprint.RowPitch;
			const uint64_t subResourceDepth = subresourceLayout.Footprint.Depth;

			// Pitch is equal to the size of a row in bytes
			const uint64_t srcPitch = mipWidth * srcTexelSize;

			// Each subresource has an offset in the memory, 
			uint8_t* dstSubresourceMem = uploadMem + subresourceLayout.Offset;

			// Go through all cubemap sides, if present
			for (uint64_t z = 0; z < subResourceDepth; ++z)
			{
				// Copy row by row
				for (uint64_t y = 0; y < subResourceHeight; ++y)
				{
					// Minimum size for a texture row in D3D12 is given by D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, which is 256
					// Data pitch might be smaller or not a multiple of this, which is why we step through the rows separately
					memcpy(dstSubresourceMem, srcMem, glm::min(subResourcePitch, srcPitch));

					dstSubresourceMem += subResourcePitch;
					srcMem += srcPitch;
				}
			}

			// Next mip width
			mipWidth = glm::max(mipWidth / 2, 1ull);
		}

	}

	for (uint32_t i = 0; i < NumSubresources; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION dest = {};
		dest.pResource = inDestResource;
		dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dest.PlacedFootprint = {};
		dest.SubresourceIndex = i;


		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = inContext.Resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layouts[i];
		src.PlacedFootprint.Offset += inContext.ResourceOffset;

		inContext.CmdList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
	}
}

// TODO: Add proper format/array/cubemap support based on above method
void UploadTexture(ID3D12Resource* inDestResource, DirectX::ScratchImage& inRes, UploadContext& inContext)
{
	const uint32_t numMips = (uint32_t)(inRes.GetImageCount());
	uint32_t NumSubresources = numMips;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * NumSubresources));
	ASSERT(layouts != nullptr);

	uint64_t* rowSize = (uint64_t*)(_alloca(sizeof(uint64_t) * NumSubresources));
	ASSERT(rowSize != nullptr);

	uint32_t* numRows = (uint32_t*)(_alloca(sizeof(uint32_t) * NumSubresources));
	ASSERT(numRows != nullptr);

	uint64_t requiredSize = 0;

	const D3D12_RESOURCE_DESC DestDesc = inDestResource->GetDesc();

	constexpr uint32_t firstSubresource = 0;
	D3D12Globals::Device->GetCopyableFootprints(&DestDesc, 0, NumSubresources, 0, layouts, numRows, rowSize, &requiredSize);

	ID3D12Resource* uploadBuffer = inContext.Resource;

	const D3D12_RESOURCE_DESC uploadBufferDesc = uploadBuffer->GetDesc();

	const bool bIsRightDimensionFormat = uploadBufferDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
	const bool bIsSmallerThanWidth = uploadBufferDesc.Width >= requiredSize + layouts[0].Offset;
	const bool bIsValidSize = requiredSize <= size_t(-1);
	const bool bIsDestRightFormat = (DestDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || (NumSubresources == 1));

	const bool isCopyValid = bIsRightDimensionFormat && bIsSmallerThanWidth & bIsValidSize && bIsDestRightFormat;

	ASSERT(isCopyValid);

	uint8_t* uploadMem = reinterpret_cast<uint8_t*>(inContext.CPUAddress);

	// Copy to the staging upload heap
	for (uint32_t i = 0; i < numMips; ++i)
	{
		ASSERT(rowSize[i] <= size_t(-1));

		const uint64_t mipId = i;

		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& mipSubresourceLayout = layouts[mipId];
		const uint64_t subResourceHeight = numRows[mipId];
		const uint64_t subResourcePitch = mipSubresourceLayout.Footprint.RowPitch;
		const uint64_t subResourceDepth = mipSubresourceLayout.Footprint.Depth;

		uint8_t* dstSubresourceMem = uploadMem + mipSubresourceLayout.Offset;

		for (uint64_t z = 0; z < subResourceDepth; ++z)
		{
			const DirectX::Image* currentSubImage = inRes.GetImage(i, 0, z);
			ASSERT(currentSubImage != nullptr);
			const uint8_t* srcSubImageTexels = currentSubImage->pixels;

			for (uint64_t y = 0; y < subResourceHeight; ++y)
			{
				const uint8_t* accessTexels = srcSubImageTexels + glm::min(subResourcePitch, currentSubImage->rowPitch);
				memcpy(dstSubresourceMem, srcSubImageTexels, glm::min(subResourcePitch, currentSubImage->rowPitch));
				dstSubresourceMem += subResourcePitch;
				srcSubImageTexels += currentSubImage->rowPitch;
			}
		}
	}

	for (uint32_t i = 0; i < NumSubresources; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION dest = {};
		dest.pResource = inDestResource;
		dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dest.PlacedFootprint = {};
		dest.SubresourceIndex = i + firstSubresource;


		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = inContext.Resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layouts[i];
		src.PlacedFootprint.Offset += inContext.ResourceOffset;

		inContext.CmdList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
	}
}

D3D12RHI::~D3D12RHI()
{

}

void D3D12RHI::Init()
{
	Instance = new D3D12RHI();
	Instance->InitPipeline();
}

// Contains DXGI_DEBUG_ALL implementation
#pragma comment(lib, "dxguid.lib") 

void D3D12RHI::Terminate()
{

#if defined(_DEBUG)
	IDXGIDebug1* pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif

	ASSERT(Instance);
	delete Instance;
}

// Returns required size of a buffer to be used for data upload
inline uint64_t GetRequiredIntermediateSize(
	ID3D12Resource* pDestinationResource,
	uint32_t FirstSubresource,
	uint32_t NumSubresources) noexcept
{
	const D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
	uint64_t RequiredSize = 0;

	ID3D12Device* device = nullptr;
	pDestinationResource->GetDevice(_uuidof(ID3D12Device), reinterpret_cast<void**>(&device));
	device->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
	device->Release();

	return RequiredSize;
}

// If the texture does not have D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS set, it needs to be double buffered and set as D3D12_RESOURCE_STATE_COMMON before and after using it on different type Queues(Graphics and Copy)
void D3D12RHI::UpdateTexture2D(eastl::shared_ptr<D3D12Texture2D>& inTexture, const uint32_t* inData, const uint32_t inWidth, const uint32_t inHeight, ID3D12GraphicsCommandList* inCommandList)
{
	ID3D12Resource* texResource = inTexture->Resource;
	const D3D12_RESOURCE_DESC textureDesc = texResource->GetDesc();
	ASSERT(textureDesc.Width == inWidth && textureDesc.Height == inHeight);

	// Get required size by device
	UINT64 uploadBufferSize = 0;
	D3D12Globals::Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

	UploadContext uploadcontext = D3D12Upload::ResourceUploadBegin(uploadBufferSize);
	UploadTexture(texResource, textureDesc.Width, textureDesc.Height, DXGI_FORMAT_R8G8B8A8_UNORM, 1, inData, uploadcontext);

	D3D12Upload::ResourceUploadEnd(uploadcontext);
}

eastl::shared_ptr<D3D12Texture2D> D3D12RHI::CreateTexture2D(const uint32_t inWidth, const uint32_t inHeight, const DXGI_FORMAT inFormat, ID3D12GraphicsCommandList* inCommandList, const eastl::wstring& inName, const void* inData, const bool bIsCubemap)
{
	eastl::shared_ptr<D3D12Texture2D> newTexture = eastl::make_shared<D3D12Texture2D>();

	ID3D12Resource* texResource;

	// Describe and create the Texture on GPU(Default Heap)
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1u;
	textureDesc.Format = inFormat;
	textureDesc.Width = inWidth;
	textureDesc.Height = inHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = bIsCubemap ? 6 : 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&D3D12Utility::GetDefaultHeapProps(),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texResource)));

	texResource->SetName(inName.c_str());

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = bIsCubemap ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1u;

	D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
	newTexture->SRVIndex = descAllocation.Index;
	for (uint32_t i = 0; i < D3D12Utility::NumFramesInFlight; ++i)
	{
		D3D12Globals::Device->CreateShaderResourceView(texResource, &srvDesc, descAllocation.CPUHandle[i]);
	}

	if (inData != nullptr)
	{
		// Get required size by device
		UINT64 uploadBufferSize = 0;
		D3D12Globals::Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
		// Same thing
		//const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureHandle, 0, 1);

		// Submit an upload request
		UploadContext uploadcontext = D3D12Upload::ResourceUploadBegin(uploadBufferSize);

		// Add buffer regions commands to Cmdlist
		//UploadTextureRaw(texResource, inData, uploadcontext, inWidth, inHeight);
		UploadTexture(texResource, inWidth, inHeight, inFormat, 1, inData, uploadcontext, bIsCubemap);


		D3D12Utility::TransitionResource(uploadcontext.CmdList, texResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);

		// Submit commands
		D3D12Upload::ResourceUploadEnd(uploadcontext);
	}

	// Transition from copy dest to shader resource
	//D3D12Utility::TransitionResource(inCommandList, texResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	newTexture->NumMips = 1u;
	newTexture->ChannelsType = ERHITextureChannelsType::RGBA;
	newTexture->NrChannels = 4;
	newTexture->Height = textureDesc.Height;
	newTexture->Width = textureDesc.Width;
	newTexture->Precision = ERHITexturePrecision::UnsignedByte;
	newTexture->Name = "Raw Texture";
	newTexture->TextureType = ETextureType::Single;
	newTexture->Resource = texResource;

	//LiveTextures.push_back(newTexture);

	return newTexture;
}


eastl::shared_ptr<D3D12Texture2D> D3D12RHI::CreateAndLoadTexture2D(const eastl::string& inDataPath, const bool inSRGB, const bool bGenerateMipMaps, ID3D12GraphicsCommandList* inCommandList)
{
	eastl::shared_ptr<D3D12Texture2D> newTexture = eastl::make_shared<D3D12Texture2D>();

	ID3D12Resource* texResource;

	DirectX::ScratchImage dxImage = {};
	DirectX::TexMetadata dxMetadata = {};

	const eastl::wstring dataPathWide = AnsiToWString(inDataPath.c_str());

	// Load the texture in cpu memory using DirectXTex
	HRESULT hresult = DirectX::LoadFromWICFile(dataPathWide.c_str(), DirectX::WIC_FLAGS_NONE, &dxMetadata, dxImage);
	const bool success = SUCCEEDED(hresult);

	ENSURE(success);

	DirectX::ScratchImage* finalImage = &dxImage;

	DirectX::ScratchImage mipMapRes;
	if (bGenerateMipMaps)
	{
		DirectX::GenerateMipMaps(*dxImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, 0, mipMapRes, false);
		finalImage = &mipMapRes;
	}

	const DXGI_FORMAT texFormat = inSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

	// Describe and create the Texture on GPU(Default Heap)
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = (uint16_t)finalImage->GetImageCount();
	textureDesc.Format = texFormat;
	textureDesc.Width = (uint32_t)dxMetadata.width;
	textureDesc.Height = (uint32_t)dxMetadata.height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&D3D12Utility::GetDefaultHeapProps(),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texResource)));

	texResource->SetName(dataPathWide.c_str());

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = (uint32_t)finalImage->GetImageCount();

	D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
	newTexture->SRVIndex = descAllocation.Index;
	for (uint32_t i = 0; i < D3D12Utility::NumFramesInFlight; ++i)
	{
		D3D12Globals::Device->CreateShaderResourceView(texResource, &srvDesc, descAllocation.CPUHandle[i]);
	}

	// Get required size by device
	UINT64 uploadBufferSize = 0;
	D3D12Globals::Device->GetCopyableFootprints(&textureDesc, 0, (uint32_t)finalImage->GetImageCount(), 0, nullptr, nullptr, nullptr, &uploadBufferSize);
	// Same thing
	//const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureHandle, 0, 1);

	// Submit an upload request
	UploadContext uploadcontext = D3D12Upload::ResourceUploadBegin(uploadBufferSize);

	// Add buffer regions commands to Cmdlist
	UploadTexture(texResource, *finalImage, uploadcontext);

	// Submit commands
	D3D12Upload::ResourceUploadEnd(uploadcontext);

	// Transition from copy dest to shader resource
	D3D12Utility::TransitionResource(inCommandList, texResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	newTexture->NumMips = (uint16_t)finalImage->GetImageCount();
	newTexture->ChannelsType = ERHITextureChannelsType::RGBA;
	newTexture->NrChannels = 4;
	newTexture->Height = textureDesc.Height;
	newTexture->Width = textureDesc.Width;
	newTexture->Precision = ERHITexturePrecision::UnsignedByte;
	newTexture->Name = inDataPath;
	newTexture->TextureType = ETextureType::Single;
	newTexture->Resource = texResource;

	LiveTextures.push_back(newTexture);

	return newTexture;
}

eastl::shared_ptr<class D3D12RenderTarget2D> D3D12RHI::CreateRenderTarget(const int32_t inWidth, const int32_t inHeight, const eastl::wstring& inName,
	const ERHITexturePrecision inPrecision, const ETextureState inInitialState, const ERHITextureFilter inFilter, const bool bInDebugViewable)
{
	eastl::shared_ptr<D3D12RenderTarget2D> newRT = eastl::make_shared<D3D12RenderTarget2D>();
	eastl::unique_ptr<D3D12Texture2D> ownedTexture = eastl::make_unique<D3D12Texture2D>();

	D3D12_RESOURCE_DESC textureDesc = {};

	DXGI_FORMAT texFormat = DXGI_FORMAT_UNKNOWN;

	switch (inPrecision)
	{
	case ERHITexturePrecision::UnsignedByte:
	{
		texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	}
	case ERHITexturePrecision::Float16:
	{
		texFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	}
	case ERHITexturePrecision::Float32:
		texFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	default:
		break;
	}

	ASSERT(texFormat != DXGI_FORMAT_UNKNOWN);

	textureDesc.Width = inWidth;
	textureDesc.Height = inHeight;
	textureDesc.MipLevels = 1;
	textureDesc.Format = texFormat;
	textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Alignment = 0;

	const D3D12_RESOURCE_STATES initState = TexStateToD3D12ResState(inInitialState);

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = texFormat;
	clearValue.Color[0] = D3D12Utility::ClearColor[0];
	clearValue.Color[1] = D3D12Utility::ClearColor[1];
	clearValue.Color[2] = D3D12Utility::ClearColor[2];
	clearValue.Color[3] = D3D12Utility::ClearColor[3];

	DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&D3D12Utility::GetDefaultHeapProps(),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		initState,
		&clearValue,
		IID_PPV_ARGS(&ownedTexture->Resource)));

	eastl::wstring textureName = L"RenderTarget_";
	textureName.append(inName);

	ownedTexture->Resource->SetName(textureName.c_str());

	// Create SRV
	{
		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
		ownedTexture->SRVIndex = descAllocation.Index;
		for (uint32_t i = 0; i < D3D12Globals::GlobalSRVHeap.NumHeaps; ++i)
		{
			D3D12Globals::Device->CreateShaderResourceView(ownedTexture->Resource, nullptr, descAllocation.CPUHandle[i]);
		}
	}

	// Create RTV
	{
		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalRTVHeap.AllocatePersistent();
		newRT->RTV = descAllocation.CPUHandle[0];

		D3D12Globals::Device->CreateRenderTargetView(ownedTexture->Resource, nullptr, newRT->RTV);
	}

	// Create RTV
	{
		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalUAVHeap.AllocatePersistent();
		newRT->UAV = descAllocation.CPUHandle[0];

		D3D12Globals::Device->CreateUnorderedAccessView(ownedTexture->Resource, nullptr, nullptr, newRT->UAV);
	}


	ownedTexture->Width = inWidth;
	ownedTexture->Height = inHeight;
	ownedTexture->Name = WStringToAnsi(textureName.c_str());

	newRT->Texture = std::move(ownedTexture);

	if (bInDebugViewable)
	{
		RenderTargets.push_back(newRT);
	}

	return newRT;
}

eastl::shared_ptr<class D3D12DepthBuffer> D3D12RHI::CreateDepthBuffer(const int32_t inWidth, const int32_t inHeight, const eastl::wstring& inName, const ETextureState inInitialState, const int32_t inArraySize)
{
	eastl::shared_ptr<D3D12DepthBuffer> newDB = eastl::make_shared<D3D12DepthBuffer>();
	eastl::unique_ptr<D3D12Texture2D> ownedTexture = eastl::make_unique<D3D12Texture2D>();

	// Based on DXGI_FORMAT_D32_FLOAT
	const DXGI_FORMAT texFormat = DXGI_FORMAT_R32_TYPELESS;
	const DXGI_FORMAT srvFormat = DXGI_FORMAT_R32_FLOAT;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = texFormat;
	textureDesc.Width = uint32_t(inWidth);
	textureDesc.Height = uint32_t(inHeight);
	textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	textureDesc.DepthOrArraySize = inArraySize;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Alignment = 0;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.f;
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;


	const D3D12_RESOURCE_STATES initState = TexStateToD3D12ResState(inInitialState);
	DXAssert(D3D12Globals::Device->CreateCommittedResource(&D3D12Utility::GetDefaultHeapProps(), D3D12_HEAP_FLAG_NONE, &textureDesc, initState, &clearValue, IID_PPV_ARGS(&ownedTexture->Resource)));

	// These need to be updated for MSAA

	// Create SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = srvFormat;

		if (inArraySize == 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}
		else if (inArraySize > 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			srvDesc.Texture2DArray.ArraySize = inArraySize;
		}

		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
		ownedTexture->SRVIndex = descAllocation.Index;
		for (uint32_t i = 0; i < D3D12Globals::GlobalSRVHeap.NumHeaps; ++i)
		{
			D3D12Globals::Device->CreateShaderResourceView(ownedTexture->Resource, &srvDesc, descAllocation.CPUHandle[i]);
		}
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

	// Create Main DSV which includes all underlying array elements, if more than one exists
	if (inArraySize == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else if (inArraySize > 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.ArraySize = uint32_t(inArraySize);
	}

	{
		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalDSVHeap.AllocatePersistent();
		newDB->DSV = descAllocation.CPUHandle[0];

		D3D12Globals::Device->CreateDepthStencilView(ownedTexture->Resource, &dsvDesc, newDB->DSV);
	}

	//// For ReadOnly DSV
	//{
	//	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	//	D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalDSVHeap.AllocatePersistent();
	//	newDB->ReadOnlyDSVIdx = descAllocation.Index;
	//	D3D12Globals::Device->CreateDepthStencilView(ownedTexture->Resource, &dsvDesc, descAllocation.CPUHandle);
	//}


	// Create DSV for each array element
	if (inArraySize > 1)
	{
		newDB->ArrayDSVs.reserve(inArraySize);
		dsvDesc.Texture2DArray.ArraySize = 1;

		for (int32_t i = 0; i < inArraySize; ++i)
		{
			dsvDesc.Texture2DArray.FirstArraySlice = uint32_t(i);
			D3D12_CPU_DESCRIPTOR_HANDLE newDSVDescr = D3D12Globals::GlobalDSVHeap.AllocatePersistent().CPUHandle[0];
			newDB->ArrayDSVs.push_back(newDSVDescr);
			D3D12Globals::Device->CreateDepthStencilView(ownedTexture->Resource, &dsvDesc, newDSVDescr);
		}
	}


	ownedTexture->Resource->SetName(inName.c_str());

	ownedTexture->Width = inWidth;
	ownedTexture->Height = inHeight;
	ownedTexture->Name = WStringToAnsi(inName.c_str());

	newDB->Texture = std::move(ownedTexture);
	newDB->ArraySize = inArraySize;

	DepthTargets.push_back(newDB);

	return newDB;
}

D3D12Texture2DCPUWritable::D3D12Texture2DCPUWritable(const uint32_t inWidth, const uint32_t inHeight, const DXGI_FORMAT inFormat, ID3D12GraphicsCommandList* inCommandList, const uint32_t* inData)
{
	for (uint32_t i = 0; i < D3D12Utility::NumFramesInFlight; ++i)
	{
		Textures[i] = D3D12RHI::Get()->CreateTexture2D(inWidth, inHeight, inFormat, inCommandList, L"CPUWritable Texture", inData);
	}
}
