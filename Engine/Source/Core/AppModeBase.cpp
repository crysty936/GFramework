#include "Core/AppModeBase.h"
#include "EngineUtils.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "Renderer/RHI/D3D12/D3D12Utility.h"
#include "Window/WindowsWindow.h"
#include "AppCore.h"
#include "Utils/IOUtils.h"
#include "Renderer/Drawable/ShapesUtils/BasicShapesData.h"
#include "backends/imgui_impl_dx12.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Camera/Camera.h"
#include "Utils/Utils.h"
#include "Utils/PerfUtils.h"


// Windows includes
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
#include "Renderer/Model/3D/Assimp/AssimpModel3D.h"

using Microsoft::WRL::ComPtr;

AppModeBase* AppModeBase::GameMode = new AppModeBase();

AppModeBase::AppModeBase()
{
	ASSERT(!GameMode);

	GameMode = this;
}

#define FRAME_BUFFERING 1

// Synchronization objects.
D3D12Fence m_fence;
D3D12Fence m_FlushGPUFence;

UINT64 m_FlushGPUFenceValue = 0;

uint64_t CurrentCPUFrame = 0;
uint64_t CurrentGPUFrame = 0;

AppModeBase::~AppModeBase()
{
	FlushGPU();
}

// Pipeline objects.

eastl::shared_ptr<D3D12IndexBuffer> ScreenQuadIndexBuffer = nullptr;
eastl::shared_ptr<D3D12VertexBuffer> ScreenQuadVertexBuffer = nullptr;

ID3D12Resource* m_BackBuffers[D3D12Globals::NumFramesInFlight];
eastl::shared_ptr<D3D12RenderTarget2D> m_GBufferAlbedo;
eastl::shared_ptr<D3D12RenderTarget2D> m_GBufferNormal;
eastl::shared_ptr<D3D12DepthBuffer> m_MainDepthBuffer;

ID3D12CommandAllocator* m_commandAllocators[D3D12Globals::NumFramesInFlight];

ID3D12RootSignature* m_GBufferMainMeshRootSignature;
ID3D12RootSignature* m_GBufferBasicObjectsRootSignature;
ID3D12RootSignature* m_LightingRootSignature;
ID3D12GraphicsCommandList* m_commandList;

ID3D12PipelineState* m_MainMeshPassPipelineState;
ID3D12PipelineState* m_BasicObjectsPipelineState;
ID3D12PipelineState* m_LightingPipelineState;

// Constant Buffer
struct MeshConstantBuffer
{
	glm::mat4 Model;
	glm::mat4 Projection;
	glm::mat4 View;
	float padding[16]; // Padding so the constant buffer is 256-byte aligned.
};
static_assert((sizeof(MeshConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

float StaticOffset = 0.f;

MeshConstantBuffer m_constantBufferData;

// One constant buffer view per signature, in the root
// One table pointing to the global SRV heap where either 
// A: all descriptors are stored and just the root for the table is modified per drawcall or
// B: Just the necessary descriptors are stored and they are copied over before the drawcall from non shader-visible heaps

// Constant Buffer is double buffered to allow modifying it each frame
// Descriptors should also be double buffered
D3D12ConstantBuffer m_constantBuffer;
uint64_t UsedCBMemory[D3D12Globals::NumFramesInFlight] = { 0 };


void AppModeBase::Init()
{
	D3D12RHI::Init();

	ImGuiInit();

	CreateInitialResources();
}

glm::mat4 MainProjection;

inline const glm::mat4& GetMainProjection()
{
	return MainProjection;
}

void AppModeBase::CreateInitialResources()
{
	BENCH_SCOPE("Create Resources");

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	// Create Main Projection
	const float CAMERA_FOV = 45.f;
	const float CAMERA_NEAR = 0.1f;
	const float CAMERA_FAR = 10000.f;

	MainProjection = glm::perspectiveLH_ZO(glm::radians(CAMERA_FOV), static_cast<float>(props.Width) / static_cast<float>(props.Height), CAMERA_NEAR, CAMERA_FAR);

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		constexpr uint32_t numRTVs = 32;
		D3D12Globals::GlobalRTVHeap.Init(false, numRTVs, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		constexpr uint32_t numSRVs = 1024;
		D3D12Globals::GlobalSRVHeap.Init(true, numSRVs, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		constexpr uint32_t numDSVs = 32;
		D3D12Globals::GlobalDSVHeap.Init(false, numDSVs, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	// Create frame resources.
	{
		// Create a RTV for each frame.
		for (UINT i = 0; i < D3D12Globals::NumFramesInFlight; i++)
		{
			// Allocate descriptor space
			D3D12DescHeapAllocationDesc newAllocation = D3D12Globals::GlobalRTVHeap.AllocatePersistent();

			// Get a reference to the swapchain buffer
			DXAssert(D3D12Globals::SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));

			// Create the descriptor at the target location in the heap
			D3D12Globals::Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, newAllocation.CPUHandle);
			eastl::wstring rtName = L"BackBuffer RenderTarget ";
			rtName += eastl::to_wstring(i);

			m_BackBuffers[i]->SetName(rtName.c_str());

			// Create the command allocator
			DXAssert(D3D12Globals::Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));

			eastl::wstring commandAllocatorName = L"CommandAllocator ";
			commandAllocatorName += eastl::to_wstring(i);

			m_commandAllocators[i]->SetName(commandAllocatorName.c_str());
		}
	}

	m_GBufferAlbedo = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"GBufferAlbedo", ERHITexturePrecision::UnsignedByte,ETextureState::Shader_Resource,  ERHITextureFilter::Nearest);
	m_GBufferNormal = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"GBufferNormal", ERHITexturePrecision::Float32, ETextureState::Shader_Resource, ERHITextureFilter::Nearest);
	m_MainDepthBuffer = D3D12RHI::Get()->CreateDepthBuffer(props.Width, props.Height, L"Main Depth Buffer");

	CreateRootSignatures();

	// Prepare Data

	CreatePSOs();
	
	// Create the command list.
	DXAssert(D3D12Globals::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[D3D12Globals::CurrentFrameIndex], m_BasicObjectsPipelineState, IID_PPV_ARGS(&m_commandList)));
	m_commandList->SetName(L"Main GFX Cmd List");

	// Memory upload test
	//for (int i = 0; i < 500; ++i)
	//{
	//	D3D12RHI::Get()->CreateAndLoadTexture2D("../Data/Textures/MinecraftGrass.jpg", /*inSRGB*/ true, m_commandList.Get());
	//}

	// Create screen quad data
	ScreenQuadIndexBuffer = D3D12RHI::Get()->CreateIndexBuffer(BasicShapesData::GetQuadIndices(), BasicShapesData::GetQuadIndicesCount());

	// Create the vertex buffer.
	{
		VertexInputLayout vbLayout;
		vbLayout.Push<float>(3, VertexInputType::Position);
		vbLayout.Push<float>(2, VertexInputType::TexCoords);

		ScreenQuadVertexBuffer = D3D12RHI::Get()->CreateVertexBuffer(vbLayout, BasicShapesData::GetQuadVertices(), BasicShapesData::GetQuadVerticesCount(), ScreenQuadIndexBuffer);
	}

	SceneManager& sManager = SceneManager::Get();
	Scene& currentScene = sManager.GetCurrentScene();

	// Cubes creation

	//eastl::shared_ptr<Model3D> TheCube = eastl::make_shared<CubeShape>("TheCube");
	//TheCube->Init(m_commandList.Get());

	//eastl::shared_ptr<CubeShape> theCube2 = eastl::make_shared<CubeShape>("TheCube2");
	//theCube2->Init(m_commandList.Get());
	//TheCube->AddChild(theCube2);
	//theCube2->Move(glm::vec3(-5.f, 0.f, 0.f));

	//currentScene.AddObject(TheCube);

	//eastl::shared_ptr<TBNQuadShape> quad = eastl::make_shared<TBNQuadShape>("TBN Quad");
	//quad->Init(m_commandList.Get());
	//currentScene.AddObject(quad);

	eastl::shared_ptr<AssimpModel3D> model= eastl::make_shared<AssimpModel3D>("../Data/Models/Sponza/Sponza.gltf", "Sponza");
	model->Init(m_commandList);

	model->Rotate(90.f, glm::vec3(0.f, 1.f, 0.f));
	model->Move(glm::vec3(0.f, -1.f, -5.f));

	//eastl::shared_ptr<AssimpModel3D> model= eastl::make_shared<AssimpModel3D>("../Data/Models/Shiba/scene.gltf", "Shiba");
	//model->Init(m_commandList);
	
	currentScene.AddObject(model);

	currentScene.GetCurrentCamera()->Move(EMovementDirection::Back, 10.f);


	// Create the constant buffer.
	{
		m_constantBuffer.Init(2 * 1024 * 1024);
		memcpy(m_constantBuffer.Map().CPUAddress, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	DXAssert(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	D3D12Globals::GraphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		CurrentCPUFrame = 0;

		m_fence.Init(CurrentCPUFrame);
		++CurrentCPUFrame;

		m_FlushGPUFence.Init(0);
		++m_FlushGPUFenceValue;

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		FlushGPU();

		// Make sure commandlist is open for object creations
		ResetFrameResources();
	}
}


void AppModeBase::CreateRootSignatures()
{
	// GBuffer Main Mesh Pass signature
	{
		D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
		// Texture
		rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		rangesPS[0].NumDescriptors = 3;
		rangesPS[0].BaseShaderRegister = 0;
		rangesPS[0].RegisterSpace = 0;
		rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
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
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
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

		m_GBufferMainMeshRootSignature = D3D12RHI::Get()->CreateRootSignature(rootSignatureDesc);
	}


	// GBuffer Basic Shapes Pass signature
	//{
	//	D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
	//	// Texture
	//	rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	rangesPS[0].NumDescriptors = 1;
	//	rangesPS[0].BaseShaderRegister = 0;
	//	rangesPS[0].RegisterSpace = 0;
	//	rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	//	rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

	//	D3D12_ROOT_PARAMETER1 rootParameters[2];

	//	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//	rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
	//	rootParameters[0].Descriptor.RegisterSpace = 0;
	//	rootParameters[0].Descriptor.ShaderRegister = 0;

	//	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	//	rootParameters[1].DescriptorTable.pDescriptorRanges = &rangesPS[0];


	//	//////////////////////////////////////////////////////////////////////////

	//	D3D12_STATIC_SAMPLER_DESC sampler = {};
	//	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	//	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	//	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	//	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	//	sampler.MipLODBias = 0;
	//	sampler.MaxAnisotropy = 0;
	//	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	//	sampler.MinLOD = 0.0f;
	//	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	//	sampler.ShaderRegister = 0;
	//	sampler.RegisterSpace = 0;
	//	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//	// Allow input layout and deny uneccessary access to certain pipeline stages.
	//	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
	//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	//		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
	//		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
	//		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	//	//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	//	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	//	rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	//	rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
	//	rootSignatureDesc.Desc_1_1.pParameters = &rootParameters[0];
	//	rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
	//	rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
	//	rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;

	//	m_GBufferBasicObjectsRootSignature = D3D12RHI::Get()->CreateRootSignature(rootSignatureDesc);
	//}

	// Final lighting root signature
	{
		D3D12_DESCRIPTOR_RANGE1 rangesPS[2];
		rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		rangesPS[0].BaseShaderRegister = 0;
		rangesPS[0].RegisterSpace = 0;
		rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

		// GBuffer Albedo and GBuffer Normal
		rangesPS[0].NumDescriptors = 2;


		D3D12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &rangesPS[0];

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

		m_LightingRootSignature = D3D12RHI::Get()->CreateRootSignature(rootSignatureDesc);
	}

}

void AppModeBase::CreatePSOs()
{
	// Mesh Pass PSO
	{
		eastl::string fullPath = "MeshPass.hlsl";
		fullPath.insert(0, "../Data/Shaders/D3D12/");

		GraphicsCompiledShaderPair meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath);

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// shader bytecodes
		D3D12_SHADER_BYTECODE vsByteCode;
		vsByteCode.pShaderBytecode = meshShaderPair.VSByteCode->GetBufferPointer();
		vsByteCode.BytecodeLength = meshShaderPair.VSByteCode->GetBufferSize();

		D3D12_SHADER_BYTECODE psByteCode;
		psByteCode.pShaderBytecode = meshShaderPair.PSByteCode->GetBufferPointer();
		psByteCode.BytecodeLength = meshShaderPair.PSByteCode->GetBufferSize();

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_GBufferMainMeshRootSignature;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;

		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::BackFaceCull);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::WriteEnabled);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_MainMeshPassPipelineState)));
	}

	// Basic Shapes PSO
	//{
	//	eastl::string fullPath = "BasicShapesMeshPass.hlsl";
	//	fullPath.insert(0, "../Data/Shaders/D3D12/");

	//	GraphicsCompiledShaderPair meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath);

	//	// Define the vertex input layout.
	//	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	//	{
	//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	//	};

	//	// shader bytecodes
	//	D3D12_SHADER_BYTECODE vsByteCode;
	//	vsByteCode.pShaderBytecode = meshShaderPair.VSByteCode->GetBufferPointer();
	//	vsByteCode.BytecodeLength = meshShaderPair.VSByteCode->GetBufferSize();

	//	D3D12_SHADER_BYTECODE psByteCode;
	//	psByteCode.pShaderBytecode = meshShaderPair.PSByteCode->GetBufferPointer();
	//	psByteCode.BytecodeLength = meshShaderPair.PSByteCode->GetBufferSize();

	//	// Describe and create the graphics pipeline state object (PSO).
	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	//	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	//	psoDesc.pRootSignature = m_GBufferBasicObjectsRootSignature.Get();
	//	psoDesc.VS = vsByteCode;
	//	psoDesc.PS = psByteCode;
	//	psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::BackFaceCull);
	//	psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
	//	psoDesc.DepthStencilState.DepthEnable = FALSE;
	//	psoDesc.DepthStencilState.StencilEnable = FALSE;
	//	psoDesc.SampleMask = UINT_MAX;
	//	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//	psoDesc.NumRenderTargets = 2;
	//	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//	psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	psoDesc.SampleDesc.Count = 1;

	//	DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_BasicObjectsPipelineState)));
	//}

	// Lighting Quad PSO
	{
		eastl::string fullPath = "LightingPass.hlsl";
		fullPath.insert(0, "../Data/Shaders/D3D12/");

		GraphicsCompiledShaderPair meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath);

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// shader bytecodes
		D3D12_SHADER_BYTECODE vsByteCode;
		vsByteCode.pShaderBytecode = meshShaderPair.VSByteCode->GetBufferPointer();
		vsByteCode.BytecodeLength = meshShaderPair.VSByteCode->GetBufferSize();

		D3D12_SHADER_BYTECODE psByteCode;
		psByteCode.pShaderBytecode = meshShaderPair.PSByteCode->GetBufferPointer();
		psByteCode.BytecodeLength = meshShaderPair.PSByteCode->GetBufferSize();

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_LightingRootSignature;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;
		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::Disabled);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::Disabled);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_LightingPipelineState)));
	}
}

void AppModeBase::SwapBuffers()
{
	D3D12Utility::TransitionResource(m_commandList, m_BackBuffers[D3D12Globals::CurrentFrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	DXAssert(m_commandList->Close());

	bCmdListOpen = false;

	ID3D12CommandList* commandLists[] = { m_commandList };
	D3D12Globals::GraphicsCommandQueue->ExecuteCommandLists(1, commandLists);

	if (GEngine->IsImguiEnabled() && ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_commandList);
	}

	D3D12Globals::SwapChain->Present(1, 0);

	D3D12Globals::CurrentFrameIndex = D3D12Globals::SwapChain->GetCurrentBackBufferIndex();

	++CurrentCPUFrame;

#if FRAME_BUFFERING
	MoveToNextFrame();
#else
	FlushGPU();
#endif

}

void AppModeBase::ResetFrameResources()
{
	if (bCmdListOpen)
	{
		return;
	}

	bCmdListOpen = true;

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	DXAssert(m_commandAllocators[D3D12Globals::CurrentFrameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	DXAssert(m_commandList->Reset(m_commandAllocators[D3D12Globals::CurrentFrameIndex], m_MainMeshPassPipelineState));
}

void AppModeBase::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//D3D12RHI::Get()->BeginFrame();
	ResetFrameResources();



	//ImGui::ShowDemoWindow();
}

uint64_t TestNrMeshesToDraw = uint64_t(-1);
uint64_t NrMeshesDrawn = 0;

void DrawMeshNodesRecursively(const eastl::vector<TransformObjPtr>& inChildNodes, const Scene& inCurrentScene)
{

	for (const TransformObjPtr& child : inChildNodes)
	{
		const TransformObject* childPtr = child.get();

		DrawMeshNodesRecursively(childPtr->GetChildren(), inCurrentScene);

		if (NrMeshesDrawn >= TestNrMeshesToDraw)
		{
			return;
		}


		if (const MeshNode* modelChild = dynamic_cast<const MeshNode*>(childPtr))
		{
			if (modelChild->Textures.size() == 0)
			{
				continue;
			}

			const glm::mat4 modelMatrix = modelChild->GetAbsoluteTransform().GetMatrix();

			//LOG_INFO("Translation for object with index %d : %f    %f    %f", i, modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);

			m_constantBufferData.Model = modelMatrix;
			m_constantBufferData.Projection = GetMainProjection();

			const glm::mat4 view = inCurrentScene.GetMainCameraLookAt();
			m_constantBufferData.View = view;

			// Use temp buffer in main constant buffer
			{
				// TODO: Abstract this

				MapResult cBufferMap = m_constantBuffer.Map();

				const uint64_t cbSize = sizeof(m_constantBufferData);

				// Used memory sizes should be aligned
				const uint64_t offset = UsedCBMemory[D3D12Globals::CurrentFrameIndex];

				// Align size
				const uint64_t finalSize = Utils::AlignTo(cbSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
				UsedCBMemory[D3D12Globals::CurrentFrameIndex] += finalSize;

				ASSERT(finalSize < m_constantBuffer.Size);

				uint8_t* CPUAddress = cBufferMap.CPUAddress;
				CPUAddress += offset;

				uint64_t GPUAddress = cBufferMap.GPUAddress;
				GPUAddress += offset;

				memcpy(CPUAddress, &m_constantBufferData, sizeof(m_constantBufferData));

				m_commandList->SetGraphicsRootConstantBufferView(0, GPUAddress);
			}

			ID3D12DescriptorHeap* ppHeaps[] = { D3D12Globals::GlobalSRVHeap.Heap };
			m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

			// Set root to index of first texture
			m_commandList->SetGraphicsRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(modelChild->Textures[0]->SRVIndex));

			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_commandList->IASetVertexBuffers(0, 1, &modelChild->VertexBuffer->VBView());
			m_commandList->IASetIndexBuffer(&modelChild->IndexBuffer->IBView());

			m_commandList->DrawIndexedInstanced(modelChild->IndexBuffer->IndexCount, 1, 0, 0, 0);

			++NrMeshesDrawn;
		}
	}
}

void AppModeBase::Draw()
{
	UsedCBMemory[D3D12Globals::CurrentFrameIndex] = 0;

	// Populate Command List

	m_commandList->SetGraphicsRootSignature(m_GBufferMainMeshRootSignature);
	m_commandList->SetPipelineState(m_MainMeshPassPipelineState);

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	static D3D12_VIEWPORT m_viewport;
	m_viewport.Width = static_cast<float>(props.Width);
	m_viewport.Height = static_cast<float>(props.Height);
	m_viewport.MinDepth = 0.f;
	m_viewport.MaxDepth = 1.f;

	m_commandList->RSSetViewports(1, &m_viewport);


	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = props.Width;
	scissorRect.bottom = props.Height;

	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Handle RTs

	D3D12Utility::TransitionResource(m_commandList, m_GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12Utility::TransitionResource(m_commandList, m_GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[2];
	renderTargets[0] = m_GBufferAlbedo->RTV;
	renderTargets[1] = m_GBufferNormal->RTV;
	m_commandList->OMSetRenderTargets(2, renderTargets, FALSE, &m_MainDepthBuffer->DSV);

	m_commandList->ClearRenderTargetView(m_GBufferAlbedo->RTV, D3D12Utility::ClearColor, 0, nullptr);
	m_commandList->ClearRenderTargetView(m_GBufferNormal->RTV, D3D12Utility::ClearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(m_MainDepthBuffer->DSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);


	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	// Draw meshes
	NrMeshesDrawn = 0;

	const eastl::vector<eastl::shared_ptr<TransformObject>>& objects = currentScene.GetAllObjects();

	for (int32_t i = 0; i < objects.size(); ++i)
	{
		// TODO: Possibly replace with RenderCommand self registration because this way it needs to be recursive and casting
		const eastl::shared_ptr<TransformObject>& currObj = objects[i];
		const eastl::shared_ptr<Model3D> currModel = eastl::dynamic_shared_pointer_cast<Model3D>(currObj);

		if (currModel.get() == nullptr)
		{
			continue;
		}

		// Record commands
		const eastl::vector<TransformObjPtr>& children = currModel->GetChildren();
		DrawMeshNodesRecursively(children, currentScene);
	}

	// Draw screen quad

	{
		ID3D12DescriptorHeap* ppHeaps[] = { D3D12Globals::GlobalSRVHeap.Heap };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		// Backbuffers are the first 2 RTVs in the Global Heap
		D3D12_CPU_DESCRIPTOR_HANDLE currentBackbufferRTDescriptor = D3D12Globals::GlobalRTVHeap.GetCPUHandle(D3D12Globals::CurrentFrameIndex);
		D3D12Utility::TransitionResource(m_commandList, m_BackBuffers[D3D12Globals::CurrentFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ClearRenderTargetView(currentBackbufferRTDescriptor, D3D12Utility::ClearColor, 0, nullptr);

		m_commandList->SetGraphicsRootSignature(m_LightingRootSignature);
		m_commandList->SetPipelineState(m_LightingPipelineState);

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
		renderTargets[0] = currentBackbufferRTDescriptor;

		m_commandList->OMSetRenderTargets(1, renderTargets, FALSE, nullptr);

		D3D12Utility::TransitionResource(m_commandList, m_GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		D3D12Utility::TransitionResource(m_commandList, m_GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_commandList->SetGraphicsRootDescriptorTable(0, D3D12Globals::GlobalSRVHeap.GetGPUHandle(m_GBufferAlbedo->Texture->SRVIndex));

		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_commandList->IASetVertexBuffers(0, 1, &ScreenQuadVertexBuffer->VBView());
		m_commandList->IASetIndexBuffer(&ScreenQuadIndexBuffer->IBView());

		m_commandList->DrawIndexedInstanced(ScreenQuadIndexBuffer->IndexCount, 1, 0, 0, 0);


	}





	if (GEngine->IsImguiEnabled())
	{
		ImGui::Begin("Scene");

		SceneManager& sManager = SceneManager::Get();
		Scene& currentScene = sManager.GetCurrentScene();
		currentScene.ImGuiDisplaySceneTree();

		ImGui::End();

		ImGui::Begin("D3D12 Settings");
		ImGui::End();
	}



}

void AppModeBase::EndFrame()
{
	//Draw ImGui
	ImGui::EndFrame();
	ImGui::Render();

	ImGuiRenderDrawData();

	SwapBuffers();

	D3D12RHI::Get()->EndFrame();
}


void AppModeBase::Terminate()
{
	D3D12RHI::Terminate();

	ASSERT(GameMode);

	delete GameMode;
	GameMode = nullptr;

	
}

void AppModeBase::Tick(float inDeltaT)
{

}

void AppModeBase::FlushGPU()
{
	m_FlushGPUFence.Signal(D3D12Globals::GraphicsCommandQueue, m_FlushGPUFenceValue);
	m_FlushGPUFence.Wait(m_FlushGPUFenceValue);

	++m_FlushGPUFenceValue;
}

// Prepare to render the next frame.
void AppModeBase::MoveToNextFrame()
{
	m_fence.Signal(D3D12Globals::GraphicsCommandQueue, CurrentCPUFrame);

	CurrentGPUFrame = m_fence.GetValue();

	const uint64_t gpuLag = CurrentCPUFrame - CurrentGPUFrame;
	if (gpuLag >= D3D12Globals::NumFramesInFlight)
	{
		// Wait for one frame
		m_fence.Wait(CurrentGPUFrame + 1);

		//LOG_WARNING("Had to wait for GPU");
	}
}

static ID3D12DescriptorHeap* m_imguiCbvSrvHeap;

void AppModeBase::ImGuiInit()
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
	DXAssert(D3D12Globals::Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_imguiCbvSrvHeap)));

	D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuHandle = m_imguiCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuHandle = m_imguiCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

	DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

	bool success = ImGui_ImplDX12_Init(D3D12Globals::Device, D3D12Globals::NumFramesInFlight, format, m_imguiCbvSrvHeap, fontSrvCpuHandle, fontSrvGpuHandle);

	ASSERT(success);

}

void AppModeBase::ImGuiRenderDrawData()
{
	// Set the imgui descriptor heap
	ID3D12DescriptorHeap* imguiHeaps[] = { m_imguiCbvSrvHeap };
	m_commandList->SetDescriptorHeaps(1, imguiHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);
}
