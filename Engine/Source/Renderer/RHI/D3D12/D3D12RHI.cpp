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
#include <dxgidebug.h>

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
#include "Core/WindowsPlatform.h"
#include "D3D12Upload.h"


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
	DXAssert(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter, false);

	ComPtr<ID3D12Device> d3d12Device;
	DXAssert(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&d3d12Device)
	));

	ComPtr<ID3D12InfoQueue> infoQueue;
	HRESULT hr = d3d12Device.As(&infoQueue);
	if (SUCCEEDED(hr))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
	}

	D3D12Globals::Device = d3d12Device.Detach();

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	DXAssert(D3D12Globals::Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D3D12Globals::GraphicsCommandQueue)));

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
	DXAssert(factory->CreateSwapChainForHwnd(
		D3D12Globals::GraphicsCommandQueue,        // Swap chain needs the queue so that it can force a flush on it.
		static_cast<HWND>(mainWindow.GetHandle()),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));


	ComPtr<IDXGISwapChain3> swapChain3;
	DXAssert(swapChain.As(&swapChain3));

	D3D12Globals::SwapChain = swapChain3.Detach();
	

	D3D12Upload::InitUpload();

}

void D3D12RHI::EndFrame()
{
	// Handle upload tasks and whatever else is necessary

	D3D12Upload::EndFrame();
	//D3D12RHI::Get()->ProcessDeferredReleases();
}

void D3D12RHI::ProcessDeferredReleases()
{
	for (ID3D12Resource* resource : DeferredReleaseResources)
	{
		resource->Release();
	}

	DeferredReleaseResources.clear();
}

