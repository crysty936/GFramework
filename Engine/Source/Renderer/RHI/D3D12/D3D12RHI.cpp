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


// Pipeline objects.
D3D12_VIEWPORT m_viewport;
D3D12_RECT m_scissorRect;
ComPtr<IDXGISwapChain3> m_swapChain;

ComPtr<ID3D12Resource> m_renderTargets[D3D12Globals::NumFramesInFlight];
eastl::shared_ptr<D3D12RenderTarget2D> m_GBufferAlbedo;

ComPtr<ID3D12CommandAllocator> m_commandAllocators[D3D12Globals::NumFramesInFlight];
ComPtr<ID3D12CommandQueue> m_commandQueue;

// Root signature
ComPtr<ID3D12RootSignature> m_rootSignature;

ComPtr<ID3D12PipelineState> m_MainMeshPipelineState;
ComPtr<ID3D12GraphicsCommandList> m_commandList;

// App resources.
eastl::shared_ptr<D3D12VertexBuffer> m_vertexBuffer;
eastl::shared_ptr<D3D12IndexBuffer> m_indexBuffer;

// Have started going towards abstraction. Another texture is now created by a CubeShape initialization. It seems to look wrong, though. Check why that is and
// continue abstracting towards standard game mode that can init stuff normally. The uniform buffer might need to go away and be replaced with specific types in materials, maybe.
// TODO NEXT
eastl::shared_ptr<D3D12Texture2D> m_texture;

// Hack to keep upload buffers referenced by command lists alive on the GPU until they finish execution
// ComPtr's are CPU objects
// GPU will be flushed at end of frame before this is cleaned up to ensure that resources are not prematurely destroyed
// TODO: Replace this hack with a copy queue ring buffer approach
eastl::vector<ComPtr<ID3D12Resource>> TextureUploadBuffers;

// Constant Buffer
struct SceneConstantBuffer
{
	glm::mat4 Model;
	glm::mat4 Projection;
	glm::mat4 View;
	float padding[16]; // Padding so the constant buffer is 256-byte aligned.
};
static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

float StaticOffset = 0.f;

SceneConstantBuffer m_constantBufferData;
D3D12Internal_ConstantBuffer m_constantBuffer;

//static uint8_t* FrameConstantBufferCPUMem[D3D12Globals::NumFramesInFlight] = {};
//static uint64_t FrameConstantBufferGPUMem[D3D12Globals::NumFramesInFlight] = {};

// Synchronization objects.
HANDLE m_fenceEvent;
ComPtr<ID3D12Fence> m_fence;

#define FRAME_BUFFERING 0

#if FRAME_BUFFERING
UINT64 m_fenceValues[D3D12Globals::NumFramesInFlight];
#else
UINT64 m_fenceValue;
#endif


struct FBrushStampBatchItemData
{
	glm::vec3 BrushPosition;
	glm::vec3 BrushExtent;
	glm::vec3 BrushScale;
	float BrushAngle;
	uint32_t BlendMode;
	uint32_t TextureIndex;


};

void InitPipeline(const WindowsWindow& inWindow)
{

	const size_t size = sizeof(FBrushStampBatchItemData);


	UINT dxgiFactoryFlags = 0;

	//#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
	//#endif

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

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	const WindowProperties& props = inWindow.GetProperties();
	m_viewport.Width = static_cast<float>(props.Width);
	m_viewport.Height = static_cast<float>(props.Height);

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = props.Width;
	m_scissorRect.bottom = props.Height;

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
		m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		static_cast<HWND>(inWindow.GetHandle()),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	D3D12Utility::DXAssert(swapChain.As(&m_swapChain));

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		constexpr uint32_t numRTVs = 32;
		D3D12Globals::GlobalRTVHeap.Init(false, numRTVs, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		constexpr uint32_t numSRVs = 1024;
		D3D12Globals::GlobalSRVHeap.Init(true, numSRVs, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Create frame resources.
	{
		// Create a RTV for each frame.
		for (UINT i = 0; i < D3D12Globals::NumFramesInFlight; i++)
		{
			// Allocate descriptor space
			D3D12DescHeapAllocationDesc newAllocation = D3D12Globals::GlobalRTVHeap.AllocatePersistent();

			// Get a reference to the swapchain buffer
			D3D12Utility::DXAssert(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));

			// Create the descriptor at the target location in the heap
			D3D12Globals::Device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, newAllocation.CPUHandle);

			// Create the command allocator
			D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
		}
	}
}

void D3D12RHI::Init_Internal()
{
	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	InitPipeline(mainWindow);

	const WindowProperties& props = mainWindow.GetProperties();

	m_GBufferAlbedo = eastl::static_shared_pointer_cast<D3D12RenderTarget2D>(CreateRenderTexture(props.Width, props.Height, ERHITexturePrecision::UnsignedByte, ERHITextureFilter::Nearest));

	// Load Assets

	// Create root signature
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(D3D12Globals::Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		//////////////////////////////////////////////////////////////////////////

		D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
		// Texture
		rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		rangesPS[0].NumDescriptors = 1;
		rangesPS[0].BaseShaderRegister = 0;
		rangesPS[0].RegisterSpace = 0;
		rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
		rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER1 rootParameters[2];

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &rangesPS[0];


		//////////////////////////////////////////////////////////////////////////

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
		rootSignatureDesc.Desc_1_1.pParameters = &rootParameters[0];
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
		rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
		rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		if (!D3D12Utility::DXAssert(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error)))
		{
			const char* errText = (char*)error->GetBufferPointer();
			LOG_ERROR("%s", errText);
		}


		D3D12Utility::DXAssert(D3D12Globals::Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	eastl::string fullPath = "shaders";
	fullPath.insert(0, "../Data/Shaders/D3D12/");
	fullPath.append(".hlsl");

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG_NAME_FOR_SOURCE;
#else																						  
	UINT compileFlags = 0;
#endif

	static eastl::string shaderCode;
	const bool readSuccess = IOUtils::TryFastReadFile(fullPath, shaderCode);

	ComPtr<ID3DBlob> vertexShader;
	ID3DBlob* vsErrBlob = nullptr;

	D3DCompile2(shaderCode.data(), shaderCode.size(), "testshadername", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, 0, nullptr, 0, &vertexShader, &vsErrBlob);


	if (!ENSURE(!vsErrBlob))
	{
		eastl::string errMessage;
		errMessage.InitialiseToSize(vsErrBlob->GetBufferSize(), '\0');
		memcpy(errMessage.data(), vsErrBlob->GetBufferPointer(), vsErrBlob->GetBufferSize());
		LOG_ERROR("%s", errMessage.c_str());
	}

	ComPtr<ID3DBlob> pixelShader;
	ID3DBlob* psErrBlob = nullptr;

	D3DCompile2(shaderCode.data(), shaderCode.size(), "testshadername", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, 0, nullptr, 0, &pixelShader, &psErrBlob);

	if (!ENSURE(!psErrBlob))
	{
		eastl::string errMessage;
		errMessage.InitialiseToSize(psErrBlob->GetBufferSize(), '\0');
		memcpy(errMessage.data(), psErrBlob->GetBufferPointer(), psErrBlob->GetBufferSize());
		LOG_ERROR("%s", errMessage.c_str());
	}


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// shader bytecodes
	D3D12_SHADER_BYTECODE vsByteCode;
	vsByteCode.pShaderBytecode = vertexShader->GetBufferPointer();
	vsByteCode.BytecodeLength = vertexShader->GetBufferSize();

	D3D12_SHADER_BYTECODE psByteCode;
	psByteCode.pShaderBytecode = pixelShader->GetBufferPointer();
	psByteCode.BytecodeLength = pixelShader->GetBufferSize();

	// Rasterizer State
	D3D12_RASTERIZER_DESC rastState;
	rastState.FillMode = D3D12_FILL_MODE_SOLID;
	rastState.CullMode = D3D12_CULL_MODE_BACK;
	//rastState.CullMode = D3D12_CULL_MODE_FRONT;
	rastState.FrontCounterClockwise = false;
	//rastState.FrontCounterClockwise = true;
	rastState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rastState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rastState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rastState.DepthClipEnable = true;
	rastState.MultisampleEnable = false;
	rastState.AntialiasedLineEnable = false;
	rastState.ForcedSampleCount = 0;
	rastState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Blend State

	D3D12_BLEND_DESC blendState;
	blendState.AlphaToCoverageEnable = FALSE;
	blendState.IndependentBlendEnable = FALSE;

	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = vsByteCode;
	psoDesc.PS = psByteCode;
	psoDesc.RasterizerState = rastState;
	psoDesc.BlendState = blendState;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	D3D12Utility::DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_MainMeshPipelineState)));

	// Create the command list.
	D3D12Utility::DXAssert(D3D12Globals::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[D3D12Globals::CurrentFrameIndex].Get(), m_MainMeshPipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

	const int32_t aspectRatio = props.Width / props.Height;

	eastl::shared_ptr<RHIIndexBuffer> newIndexBuffer = CreateIndexBuffer(BasicShapesData::GetCubeIndices(), BasicShapesData::GetCubeIndicesCount());
	m_indexBuffer = eastl::static_shared_pointer_cast<D3D12IndexBuffer>(newIndexBuffer);

	// Create the vertex buffer.
	{
		VertexInputLayout vbLayout;
		// Vertex points
		vbLayout.Push<float>(3, VertexInputType::Position);
		// Vertex Tex Coords
		vbLayout.Push<float>(3, VertexInputType::Normal);
		vbLayout.Push<float>(2, VertexInputType::TexCoords);

		eastl::shared_ptr<RHIVertexBuffer> newVertexBuffer = CreateVertexBuffer(vbLayout, BasicShapesData::GetCubeVertices(), BasicShapesData::GetCubeVerticesCount(), m_indexBuffer);
		m_vertexBuffer = eastl::static_shared_pointer_cast<D3D12VertexBuffer>(newVertexBuffer);
	}

	// Create the constant buffer.
	{
		m_constantBuffer.Init(2 * 1024 * 1024);
		memcpy(m_constantBuffer.Map().CPUAddress, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	m_texture = eastl::static_shared_pointer_cast<D3D12Texture2D>(CreateAndLoadTexture2D("../Data/Textures/MinecraftGrass.jpg", /*inSRGB*/ true));

	D3D12Utility::DXAssert(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
#if FRAME_BUFFERING
		m_fenceValues[0] = m_fenceValues[1] = 1;

		D3D12Utility::DXAssert(D3D12Globals::Device->CreateFence(m_fenceValues[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValues[0]++;
#else
		D3D12Utility::DXAssert(D3D12Globals::Device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue++;
#endif

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			D3D12Utility::DXAssert(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForPreviousFrame();

		// Make sure commandlist is open for object creations
		BeginFrame();
	}
}

// Returns required size of a buffer to be used for data upload
inline UINT64 GetRequiredIntermediateSize(
	ID3D12Resource* pDestinationResource,
	UINT FirstSubresource,
	UINT NumSubresources) noexcept
{
	const D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
	UINT64 RequiredSize = 0;

	ID3D12Device* device = nullptr;
	pDestinationResource->GetDevice(_uuidof(ID3D12Device), reinterpret_cast<void**>(&device));
	device->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
	device->Release();

	return RequiredSize;
}

void UploadTextureBlocking(ID3D12Resource* inDestResource, ID3D12Resource* inUploadResource, const D3D12_SUBRESOURCE_DATA* inSrcData, uint32_t NumSubresources, ID3D12GraphicsCommandList* inCmdList, DirectX::ScratchImage& inRes);

// One constant buffer view per signature, in the root
// One table pointing to the global SRV heap where either 
// A: all descriptors are stored and just the root for the table is modified per drawcall or
// B: Just the necessary descriptors are stored and they are copied over before the drawcall from non shader-visible heaps

// Constant Buffer is double buffered to allow modifying it each frame
// Descriptors should also be double buffered

D3D12RHI::D3D12RHI()
{
}

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
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void D3D12RHI::Init()
{
	Instance = new D3D12RHI();
	Instance->Init_Internal();
}

void D3D12RHI::Terminate()
{
	ASSERT(Instance);
	delete Instance;
}

#if FRAME_BUFFERING
void D3D12RHI::WaitForPreviousFrame()
{
	const UINT64 currentFrameRequiredFenceValue = m_fenceValues[D3D12Globals::CurrentFrameIndex];
	// We want that fence to be set to that value from the GPU side
	D3D12Utility::DXAssert(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[D3D12Globals::CurrentFrameIndex]));

	// Tell m_fence to raise this event once it's equal fence value
	D3D12Utility::DXAssert(m_fence->SetEventOnCompletion(m_fenceValues[D3D12Globals::CurrentFrameIndex], m_fenceEvent));

	// Wait until that event is raised
	WaitForSingleObject(m_fenceEvent, INFINITE);

	m_fenceValues[D3D12Globals::CurrentFrameIndex]++;
}
#else
void D3D12RHI::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.


	// Basically tells the GPU that it should set that fence to that value.
	// Because of the way the queue works, that command is only going to be executed once all other command lists in the queue are done

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	// We want that fence to be set to that value from the GPU side
	D3D12Utility::DXAssert(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	const UINT64 fenceValue = m_fence->GetCompletedValue();
	if (fenceValue < fence)
	{
		// Tell m_fence to raise this event once it's equal fence value
		D3D12Utility::DXAssert(m_fence->SetEventOnCompletion(fence, m_fenceEvent));

		// Wait until that event is raised
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	D3D12Globals::CurrentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Clean all used up upload buffers
	TextureUploadBuffers.clear();
}
#endif


#if FRAME_BUFFERING
// Prepare to render the next frame.
void D3D12RHI::MoveToNextFrame()
{
	const UINT64 submittedFrameFenceValue = m_fenceValues[D3D12Globals::CurrentFrameIndex];

	// Place signal for frame that was just submitted
	D3D12Utility::DXAssert(m_commandQueue->Signal(m_fence.Get(), submittedFrameFenceValue));

	// Move onto next frame. Backbuffer index was changed as Present was called before this
	D3D12Globals::CurrentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

	const UINT64 presentFenceValue = m_fence->GetCompletedValue();

	// Happens if Graphics device is removed while running
	if (presentFenceValue == UINT64_MAX)
	{
		__debugbreak();
	}

	const UINT64 toStartFrameFenceValue = m_fenceValues[D3D12Globals::CurrentFrameIndex];

	if (presentFenceValue < toStartFrameFenceValue)
	{
		D3D12Utility::DXAssert(m_fence->SetEventOnCompletion(toStartFrameFenceValue, m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, false);
	}

	m_fenceValues[D3D12Globals::CurrentFrameIndex] = submittedFrameFenceValue + 1;
}
#endif

void D3D12RHI::Test()
{
	ImGui::Begin("D3D12 Settings");
	static glm::vec3 translation(0.f, 0.f, 2.f);
	ImGui::DragFloat3("Obj Translation", &translation.x, 0.1f);

	ImGui::End();

	const float translationSpeed = 0.005f;
	const float offsetBounds = 1.25f;

	StaticOffset += translationSpeed;
	if (StaticOffset > offsetBounds)
	{
		StaticOffset = -offsetBounds;
	}

	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::translate(modelMatrix, translation);
	modelMatrix = glm::rotate(modelMatrix, StaticOffset * StaticOffset, glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
	m_constantBufferData.Model = modelMatrix;

	const float windowWidth = static_cast<float>(GEngine->GetMainWindow().GetProperties().Width);
	const float windowHeight = static_cast<float>(GEngine->GetMainWindow().GetProperties().Height);
	const float CAMERA_FOV = 45.f;
	const float CAMERA_NEAR = 0.1f;
	const float CAMERA_FAR = 10000.f;

	glm::mat4 projection = glm::perspectiveLH_ZO(glm::radians(CAMERA_FOV), windowWidth / windowHeight, CAMERA_NEAR, CAMERA_FAR);

	m_constantBufferData.Projection = projection;

	const glm::mat4 view = SceneManager::Get().GetCurrentScene().GetCurrentCamera()->GetLookAt();

	m_constantBufferData.View = view;

	MapResult cBufferMap = m_constantBuffer.Map();

	memcpy(cBufferMap.CPUAddress, &m_constantBufferData, sizeof(m_constantBufferData));

	// Populate Command List

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_commandList->SetGraphicsRootConstantBufferView(0, cBufferMap.GPUAddress);

	// Desc Heap
	ID3D12DescriptorHeap* ppHeaps[] = { D3D12Globals::GlobalSRVHeap.Heap };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(m_texture->SRVIndex));

	D3D12Utility::TransitionResource(m_commandList.Get(), m_renderTargets[D3D12Globals::CurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[2];

	// Backbuffers are the first 2 RTVs in the Global Heap
	D3D12_CPU_DESCRIPTOR_HANDLE currentBackbufferRTDescriptor = D3D12Globals::GlobalRTVHeap.GetCPUHandle(D3D12Globals::CurrentFrameIndex);

	renderTargets[0] = currentBackbufferRTDescriptor; // TODO: This has to be removed from the RTs and stuff has to be copied into the backbuffer at the end
	renderTargets[1] = m_GBufferAlbedo->RTV;

	m_commandList->OMSetRenderTargets(2, renderTargets, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(currentBackbufferRTDescriptor, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer->VBView());
	m_commandList->IASetIndexBuffer(&m_indexBuffer->IBView());

	m_commandList->DrawIndexedInstanced(BasicShapesData::GetCubeIndicesCount(), 1, 0, 0, 0);

}

ComPtr<ID3D12DescriptorHeap> m_imguiCbvSrvHeap;

void D3D12RHI::ImGuiInit()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(static_cast<HWND>(GEngine->GetMainWindow().GetHandle()));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	D3D12Utility::DXAssert(D3D12Globals::Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_imguiCbvSrvHeap)));

	D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuHandle = m_imguiCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuHandle = m_imguiCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

	DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;


	bool success = ImGui_ImplDX12_Init(D3D12Globals::Device, D3D12Globals::NumFramesInFlight, format, m_imguiCbvSrvHeap.Get(), fontSrvCpuHandle, fontSrvGpuHandle);

	ASSERT(success);

}

void D3D12RHI::ImGuiBeginFrame()
{
	ImGui_ImplDX12_NewFrame();
}

void D3D12RHI::ImGuiRenderDrawData()
{
	// Set the imgui descriptor heap
	ID3D12DescriptorHeap* imguiHeaps[] = { m_imguiCbvSrvHeap.Get() };
	m_commandList->SetDescriptorHeaps(1, imguiHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
}

void D3D12RHI::SwapBuffers()
{
	D3D12Utility::TransitionResource(m_commandList.Get(), m_renderTargets[D3D12Globals::CurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	D3D12Utility::DXAssert(m_commandList->Close());

	bCmdListOpen = false;

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(1, commandLists);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_commandList.Get());
	}

	m_swapChain->Present(1, 0);


#if FRAME_BUFFERING
	MoveToNextFrame();
#else
	WaitForPreviousFrame();
#endif

}


// TODO
// Can't move this over because of the TextureUploadBuffers hack
// Fix the hack and move to other file
eastl::shared_ptr<D3D12Texture2D> D3D12RHI::CreateAndLoadTexture2D(const eastl::string& inDataPath, const bool inSRGB)
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


	DirectX::ScratchImage dxImageTest = {};
	DirectX::TexMetadata dxMetadataTest = {};

	const int32_t pathLength = inDataPath.length();
	wchar_t* wideString = new wchar_t[pathLength + 1];
	for (int32_t i = 0; i < pathLength; ++i)
	{
		wideString[i] = inDataPath[i];
	}
	wideString[pathLength] = L'\0';

	HRESULT hresult = DirectX::LoadFromWICFile(wideString, DirectX::WIC_FLAGS_NONE, &dxMetadataTest, dxImageTest);
	const bool success = SUCCEEDED(hresult);

	ENSURE(success);

	DirectX::ScratchImage res;
	DirectX::GenerateMipMaps(*dxImageTest.GetImage(0, 0, 0), DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, 0, res, false);

	// Describe and create a Texture2D on GPU(Default Heap)
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = (uint16_t)res.GetImageCount();
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = (uint32_t)dxMetadataTest.width;
	textureDesc.Height = (uint32_t)dxMetadataTest.height;
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
	UploadTextureBlocking(texResource, textureUploadHeap, 1, m_commandList.Get(), res);

	// Transition from copy dest to shader resource
	D3D12Utility::TransitionResource(m_commandList.Get(), texResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

eastl::shared_ptr<class D3D12RenderTarget2D> D3D12RHI::CreateRenderTexture(const int32_t inWidth, const int32_t inHeight, const ERHITexturePrecision inPrecision /*= ERHITexturePrecision::UnsignedByte*/, const ERHITextureFilter inFilter /*= ERHITextureFilter::Linear*/)
{
	eastl::shared_ptr<D3D12RenderTarget2D> newRT = eastl::make_shared<D3D12RenderTarget2D>();
	eastl::unique_ptr<D3D12Texture2D> ownedTexture = eastl::make_unique<D3D12Texture2D>();

 	D3D12_RESOURCE_DESC textureDesc = {};
 
 	DXGI_FORMAT texFormat;
 
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
 
 	eastl::wstring textureName = L"RenderTarget ";
 	const eastl::wstring textureIndex = eastl::to_wstring(RenderTargetIndex);
 	textureName += textureIndex;
 
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

void D3D12RHI::BeginFrame()
{
	if (bCmdListOpen)
	{
		return;
	}

	bCmdListOpen = true;

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	D3D12Utility::DXAssert(m_commandAllocators[D3D12Globals::CurrentFrameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	D3D12Utility::DXAssert(m_commandList->Reset(m_commandAllocators[D3D12Globals::CurrentFrameIndex].Get(), m_MainMeshPipelineState.Get()));
}


