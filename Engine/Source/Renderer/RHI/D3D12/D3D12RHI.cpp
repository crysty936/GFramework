#include "D3D12RHI.h"

#include "Core/EngineUtils.h"
#include "Core/AppCore.h"
#include "Window/WindowsWindow.h"
#include "Utils/IOUtils.h"

// Exclude rarely-used stuff from Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             
#endif
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <DirectXMath.h>
//#include "glm/ext/vector_common.inl"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"


#include "DirectXTex.h"
#include "Utils/ImageLoading.h"
#include "Renderer/Drawable/ShapesUtils/BasicShapesData.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

#include "Scene/SceneManager.h"

#include "Scene/Scene.h"
#include "Camera/Camera.h"
#include "D3D12Utility.h"
#include "D3D12GraphicsTypes_Internal.h"
#include "D3D12Resources.h"


// D3D12 RHI stuff to do:
// Fix the default memory allocation to use a ring buffer instead of the hack that is present right now
// Modify the constant buffers to allow a single buffer to be used for all draws

using Microsoft::WRL::ComPtr;

inline eastl::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return eastl::string(s_str);
}

void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;

	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}



// Hack to keep upload buffers referenced by command lists alive on the GPU until they finish execution
// ComPtr's are CPU objects
// GPU will be flushed at end of frame before this is cleaned up to ensure that resources are not prematurely destroyed
// TODO: Replace this hack with a copy queue ring buffer approach
eastl::vector<ComPtr<ID3D12Resource>> TextureUploadBuffers;


void D3D12RHI::InitPipeline()
{
	UINT dxgiFactoryFlags = 0;

	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
	#endif

	ComPtr<IDXGIFactory4> factory;
	D3D12Utility::DXAssert(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter, false);

	D3D12Utility::DXAssert(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&D3D12Globals::Device)
	));


	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D3D12Globals::CommandQueue)));

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = D3D12Globals::NumFramesInFlight;
	swapChainDesc.Width = props.Width;
	swapChainDesc.Height = props.Height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	D3D12Utility::DXAssert(factory->CreateSwapChainForHwnd(
		D3D12Globals::CommandQueue,        // Swap chain needs the queue so that it can force a flush on it.
		static_cast<HWND>(mainWindow.GetHandle()),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));


	ComPtr<IDXGISwapChain3> swapChain3;
	D3D12Utility::DXAssert(swapChain.As(&swapChain3));

	D3D12Globals::SwapChain = swapChain3.Detach();
	


}


void UploadTextureBlocking(ID3D12Resource* inDestResource, ID3D12Resource* inUploadResource, const D3D12_SUBRESOURCE_DATA* inSrcData, uint32_t NumSubresources, ID3D12GraphicsCommandList* inCmdList, DirectX::ScratchImage& inRes);

// One constant buffer view per signature, in the root
// One table pointing to the global SRV heap where either 
// A: all descriptors are stored and just the root for the table is modified per drawcall or
// B: Just the necessary descriptors are stored and they are copied over before the drawcall from non shader-visible heaps

// Constant Buffer is double buffered to allow modifying it each frame
// Descriptors should also be double buffered

void UploadTextureBlocking(ID3D12Resource* inDestResource, ID3D12Resource* inUploadResource, uint32_t NumSubresources, ID3D12GraphicsCommandList* inCmdList, DirectX::ScratchImage& inRes)
{
	const uint32_t numMips = (uint32_t)(inRes.GetImageCount());
	const DirectX::TexMetadata& metaData = inRes.GetMetadata();
	NumSubresources = numMips;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * NumSubresources));
	ASSERT(layouts != nullptr);

	uint64_t* rowSize = (uint64_t*)(_alloca(sizeof(uint64_t) * NumSubresources));
	ASSERT(rowSize != nullptr);

	uint32_t* numRows = (uint32_t*)(_alloca(sizeof(uint32_t) * NumSubresources));
	ASSERT(numRows != nullptr);

	uint64_t requiredSize = 0;

	const D3D12_RESOURCE_DESC DestDesc = inDestResource->GetDesc();
	ID3D12Device* device = nullptr;
	inDestResource->GetDevice(_uuidof(ID3D12Device), reinterpret_cast<void**>(&device));

	constexpr uint32_t firstSubresource = 0;
	device->GetCopyableFootprints(&DestDesc, 0, NumSubresources, 0, layouts, numRows, rowSize, &requiredSize);
	device->Release();

	const D3D12_RESOURCE_DESC uploadBufferDesc = inUploadResource->GetDesc();

	const bool isCopyValid =
		uploadBufferDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
		uploadBufferDesc.Width >= requiredSize + layouts[0].Offset &&
		requiredSize <= size_t(-1) &&
		(DestDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || (NumSubresources == 1));

	ASSERT(isCopyValid);

	char* uploadResourceCPUMem = nullptr;

	HRESULT hr = inUploadResource->Map(0, nullptr, reinterpret_cast<void**>(&uploadResourceCPUMem));

	uint8_t* uploadMem = reinterpret_cast<uint8_t*>(uploadResourceCPUMem);

	if (!ASSERT(SUCCEEDED(hr)))
	{
		return;
	}

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
				uint8_t accessTexel = *accessTexels;
				memcpy(dstSubresourceMem, srcSubImageTexels, glm::min(subResourcePitch, currentSubImage->rowPitch));
				dstSubresourceMem += subResourcePitch;
				srcSubImageTexels += currentSubImage->rowPitch;
			}
		}
	}

	inUploadResource->Unmap(0, nullptr);

	if (DestDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		inCmdList->CopyBufferRegion(inDestResource, 0, inUploadResource, layouts[0].Offset, layouts[0].Footprint.Width);
	}
	else
	{
		for (uint32_t i = 0; i < NumSubresources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION dest = {};
			dest.pResource = inDestResource;
			dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dest.PlacedFootprint = {};
			dest.SubresourceIndex = i + firstSubresource;


			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = inUploadResource;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = layouts[i];

			inCmdList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
		}
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

void D3D12RHI::Terminate()
{
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


// TODO
// Can't move this over because of the TextureUploadBuffers hack
// Fix the hack and move to other file
eastl::shared_ptr<D3D12Texture2D> D3D12RHI::CreateAndLoadTexture2D(const eastl::string& inDataPath, const bool inSRGB, ID3D12GraphicsCommandList* inCommandList)
{
	eastl::shared_ptr<D3D12Texture2D> newTexture = eastl::make_shared<D3D12Texture2D>();

	ComPtr<ID3D12Resource>& newTextureUploadHeap = TextureUploadBuffers.emplace_back();

	ID3D12Resource* textureUploadHeap = newTextureUploadHeap.Get();
	ID3D12Resource* texResource;

	D3D12_HEAP_PROPERTIES UploadHeapProps;
	UploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	UploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	UploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	UploadHeapProps.CreationNodeMask = 1;
	UploadHeapProps.VisibleNodeMask = 1;


	DirectX::ScratchImage dxImage = {};
	DirectX::TexMetadata dxMetadata = {};

	const int32_t pathLength = inDataPath.length();
	wchar_t* wideString = new wchar_t[pathLength + 1];
	for (int32_t i = 0; i < pathLength; ++i)
	{
		wideString[i] = inDataPath[i];
	}
	wideString[pathLength] = L'\0';

	HRESULT hresult = DirectX::LoadFromWICFile(wideString, DirectX::WIC_FLAGS_NONE, &dxMetadata, dxImage);
	const bool success = SUCCEEDED(hresult);

	ENSURE(success);

	DirectX::ScratchImage res;
	DirectX::GenerateMipMaps(*dxImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, 0, res, false);

	// Describe and create a Texture2D on GPU(Default Heap)
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = (uint16_t)res.GetImageCount();
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = (uint32_t)dxMetadata.width;
	textureDesc.Height = (uint32_t)dxMetadata.height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&D3D12Utility::GetDefaultHeapProps(),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texResource)));

	// Get required size by device
	UINT64 uploadBufferSize = 0;
	D3D12Globals::Device->GetCopyableFootprints(&textureDesc, 0, (uint32_t)res.GetImageCount(), 0, nullptr, nullptr, nullptr, &uploadBufferSize);

	// Same thing
	//const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureHandle, 0, 1);

	D3D12_RESOURCE_DESC UploadBufferDesc;
	UploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	UploadBufferDesc.Alignment = 0;
	UploadBufferDesc.Width = uploadBufferSize;
	UploadBufferDesc.Height = 1;
	UploadBufferDesc.DepthOrArraySize = 1;
	UploadBufferDesc.MipLevels = 1;
	UploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	UploadBufferDesc.SampleDesc.Count = 1;
	UploadBufferDesc.SampleDesc.Quality = 0;
	UploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	UploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// Create the CPU -> GPU Upload Buffer
	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
		&UploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&UploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	// Blocking call
	UploadTextureBlocking(texResource, textureUploadHeap, 1, inCommandList, res);

	// Transition from copy dest to shader resource
	D3D12Utility::TransitionResource(inCommandList, texResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = (uint32_t)res.GetImageCount();

	D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
	newTexture->SRVIndex = descAllocation.Index;
	D3D12Globals::Device->CreateShaderResourceView(texResource, &srvDesc, descAllocation.CPUHandle);

	newTexture->NumMips = (uint16_t)res.GetImageCount();
	newTexture->ChannelsType = ERHITextureChannelsType::RGBA;
	newTexture->NrChannels = 4;
	newTexture->Height = textureDesc.Height;
	newTexture->Width = textureDesc.Width;
	newTexture->Precision = ERHITexturePrecision::UnsignedByte;
	newTexture->SourcePath = inDataPath;
	newTexture->TextureType = ETextureType::Single;
	newTexture->Resource = texResource;

	return newTexture;
}

eastl::shared_ptr<class D3D12RenderTarget2D> D3D12RHI::CreateRenderTexture(const int32_t inWidth, const int32_t inHeight, const eastl::wstring& inName,
	const ERHITexturePrecision inPrecision /*= ERHITexturePrecision::UnsignedByte*/, const ERHITextureFilter inFilter /*= ERHITextureFilter::Linear*/)
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
 	default:
 		break;
 	}

	ASSERT(texFormat != DXGI_FORMAT_UNKNOWN);
 
 	textureDesc.Width = inWidth;
 	textureDesc.Height = inHeight;
 	textureDesc.MipLevels = 1;
 	textureDesc.Format = texFormat;
 	textureDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
 	textureDesc.DepthOrArraySize = 1;
 	textureDesc.SampleDesc.Count = 1;
 	textureDesc.SampleDesc.Quality = 0;
 	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
 	textureDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
 	textureDesc.Alignment = 0;
 
 	const D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
 
 	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommittedResource(
 		&D3D12Utility::GetDefaultHeapProps(),
 		D3D12_HEAP_FLAG_NONE,
 		&textureDesc,
 		initState,
 		nullptr,
 		IID_PPV_ARGS(&ownedTexture->Resource)));
 
 	static int32_t RenderTargetIndex = 0;
 	++RenderTargetIndex;
 
 	eastl::wstring textureName = L"RenderTarget_";
 	textureName.append(inName);
 
	ownedTexture->Resource->SetName(textureName.c_str());
 
 	// Create SRV
 	{
 		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalSRVHeap.AllocatePersistent();
		ownedTexture->SRVIndex = descAllocation.Index;
 		D3D12Globals::Device->CreateShaderResourceView(ownedTexture->Resource, nullptr, descAllocation.CPUHandle);
 	}
 
 	// Create RTV
 	{
 		D3D12DescHeapAllocationDesc descAllocation = D3D12Globals::GlobalRTVHeap.AllocatePersistent();
 		newRT->RTV = descAllocation.CPUHandle;
 
 		D3D12Globals::Device->CreateRenderTargetView(ownedTexture->Resource, nullptr, descAllocation.CPUHandle);
 
 	}
 
 	ownedTexture->Width = inWidth;
	ownedTexture->Height = inHeight;

	newRT->Texture = std::move(ownedTexture);

	return newRT;
}

void D3D12RHI::DoTextureUploadHack()
{
	TextureUploadBuffers.clear();
}

