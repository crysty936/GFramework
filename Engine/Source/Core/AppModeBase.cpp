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

using Microsoft::WRL::ComPtr;

AppModeBase* AppModeBase::GameMode = new AppModeBase();

AppModeBase::AppModeBase()
{
	ASSERT(!GameMode);

	GameMode = this;
}

// Synchronization objects.
HANDLE m_fenceEvent;
ComPtr<ID3D12Fence> m_fence;

#define FRAME_BUFFERING 1

#if FRAME_BUFFERING
UINT64 m_fenceValues[D3D12Globals::NumFramesInFlight];
#else
UINT64 m_fenceValue;
#endif

AppModeBase::~AppModeBase()
{
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

// Pipeline objects.

ComPtr<ID3D12Resource> m_BackBuffers[D3D12Globals::NumFramesInFlight];
eastl::shared_ptr<D3D12RenderTarget2D> m_GBufferAlbedo;

ComPtr<ID3D12CommandAllocator> m_commandAllocators[D3D12Globals::NumFramesInFlight];

ComPtr<ID3D12RootSignature> m_rootSignature;
ComPtr<ID3D12GraphicsCommandList> m_commandList;

ComPtr<ID3D12PipelineState> m_MainMeshPipelineState;

eastl::shared_ptr<Model3D> TheCube;

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

void AppModeBase::CreateInitialResources()
{
	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

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
			DXAssert(D3D12Globals::SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));

			// Create the descriptor at the target location in the heap
			D3D12Globals::Device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, newAllocation.CPUHandle);

			// Create the command allocator
			DXAssert(D3D12Globals::Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
		}
	}

	m_GBufferAlbedo = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"GBufferAlbedo", ERHITexturePrecision::UnsignedByte, ERHITextureFilter::Nearest);

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
		if (!DXAssert(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error)))
		{
			const char* errText = (char*)error->GetBufferPointer();
			LOG_ERROR("%s", errText);
		}


		DXAssert(D3D12Globals::Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
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

	DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_MainMeshPipelineState)));

	// Create the command list.
	DXAssert(D3D12Globals::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[D3D12Globals::CurrentFrameIndex].Get(), m_MainMeshPipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

	// Cube creation

	TheCube = eastl::make_shared<CubeShape>("TheCube");
	TheCube->Init(m_commandList.Get());

	eastl::shared_ptr<CubeShape> theCube2 = eastl::make_shared<CubeShape>("TheCube2");
	theCube2->Init(m_commandList.Get());


	SceneManager& sManager = SceneManager::Get();
	Scene& currentScene = sManager.GetCurrentScene();

	currentScene.GetCurrentCamera()->Move(EMovementDirection::Back, 10.f);

	TheCube->Move(glm::vec3(-5.f, 0.f, 0.f));

	currentScene.AddObject(TheCube);
	currentScene.AddObject(theCube2);

	// Create the constant buffer.
	{
		m_constantBuffer.Init(2 * 1024 * 1024);
		memcpy(m_constantBuffer.Map().CPUAddress, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	DXAssert(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	D3D12Globals::CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
#if FRAME_BUFFERING
		m_fenceValues[0] = m_fenceValues[1] = 1;

		DXAssert(D3D12Globals::Device->CreateFence(m_fenceValues[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValues[0]++;
#else
		DXAssert(D3D12Globals::Device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue++;
#endif

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			DXAssert(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForPreviousFrame();

		// Make sure commandlist is open for object creations
		ResetFrameResources();
	}
}


void AppModeBase::SwapBuffers()
{
	D3D12Utility::TransitionResource(m_commandList.Get(), m_BackBuffers[D3D12Globals::CurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	DXAssert(m_commandList->Close());

	bCmdListOpen = false;

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	D3D12Globals::CommandQueue->ExecuteCommandLists(1, commandLists);

	if (GEngine->IsImguiEnabled() && ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_commandList.Get());
	}

	D3D12Globals::SwapChain->Present(1, 0);


#if FRAME_BUFFERING
	MoveToNextFrame();
#else
	WaitForPreviousFrame();
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
	DXAssert(m_commandList->Reset(m_commandAllocators[D3D12Globals::CurrentFrameIndex].Get(), m_MainMeshPipelineState.Get()));
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

D3D12_VIEWPORT m_viewport;

void AppModeBase::Draw()
{
	UsedCBMemory[D3D12Globals::CurrentFrameIndex] = 0;

	if (GEngine->IsImguiEnabled())
	{
		ImGui::Begin("Scene");

		SceneManager& sManager = SceneManager::Get();
		Scene& currentScene = sManager.GetCurrentScene();
		currentScene.DisplayObjects();

		ImGui::End();

		ImGui::Begin("D3D12 Settings");
		ImGui::End();
	}

	// Populate Command List

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();
	m_viewport.Width = static_cast<float>(props.Width);
	m_viewport.Height = static_cast<float>(props.Height);

	m_commandList->RSSetViewports(1, &m_viewport);


	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = props.Width;
	scissorRect.bottom = props.Height;

	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Handle RTs

	// Backbuffers are the first 2 RTVs in the Global Heap
	D3D12_CPU_DESCRIPTOR_HANDLE currentBackbufferRTDescriptor = D3D12Globals::GlobalRTVHeap.GetCPUHandle(D3D12Globals::CurrentFrameIndex);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(currentBackbufferRTDescriptor, clearColor, 0, nullptr);
	m_commandList->ClearRenderTargetView(m_GBufferAlbedo->RTV, clearColor, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[2];
	renderTargets[0] = currentBackbufferRTDescriptor; // TODO: This has to be removed from the RTs and stuff has to be rendered/copied into the backbuffer at the end
	renderTargets[1] = m_GBufferAlbedo->RTV;

	m_commandList->OMSetRenderTargets(2, renderTargets, FALSE, nullptr);
	D3D12Utility::TransitionResource(m_commandList.Get(), m_BackBuffers[D3D12Globals::CurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SceneManager& sManager = SceneManager::Get();
	Scene& currentScene = sManager.GetCurrentScene();

	// Draw meshes
	const eastl::vector<eastl::shared_ptr<TransformObject>>& objects = currentScene.GetAllObjects();

	for (int32_t i = 0; i < objects.size(); ++i)
	{
		// TODO: Replace with proper RenderCommand register and brouped draw because this way it needs to be recursive and casting, etc. It doesn't work properly
		const eastl::shared_ptr<TransformObject>& currObj = objects[i];
		const eastl::shared_ptr<Model3D> currModel = eastl::static_shared_pointer_cast<Model3D>(currObj);

		if (currModel.get() == nullptr)
		{
			continue;
		}

		glm::mat4 modelMatrix = currModel->GetAbsoluteTransform().GetMatrix();

		//LOG_INFO("Translation for object with index %d : %f    %f    %f", i, modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);

		m_constantBufferData.Model = modelMatrix;

		const float windowWidth = static_cast<float>(GEngine->GetMainWindow().GetProperties().Width);
		const float windowHeight = static_cast<float>(GEngine->GetMainWindow().GetProperties().Height);
		const float CAMERA_FOV = 45.f;
		const float CAMERA_NEAR = 0.1f;
		const float CAMERA_FAR = 10000.f;

		glm::mat4 projection = glm::perspectiveLH_ZO(glm::radians(CAMERA_FOV), windowWidth / windowHeight, CAMERA_NEAR, CAMERA_FAR);
		const glm::mat4 view = currentScene.GetCurrentCamera()->GetLookAt();

		m_constantBufferData.Projection = projection;
		m_constantBufferData.View = view;

		// Use temp buffer in main constant buffer
		{
			// TODO: Abstract this

			MapResult cBufferMap = m_constantBuffer.Map();

			const uint64_t cbSize = sizeof(m_constantBufferData);
			constexpr uint64_t constantBufferAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

			// Used memory sizes should be aligned
			const uint64_t offset = UsedCBMemory[D3D12Globals::CurrentFrameIndex];

			// Align size
			const uint64_t finalSize = ((cbSize + constantBufferAlignment - 1) / constantBufferAlignment) * constantBufferAlignment;
			UsedCBMemory[D3D12Globals::CurrentFrameIndex] += finalSize;

			ASSERT(finalSize < m_constantBuffer.Size);

			uint8_t* CPUAddress = cBufferMap.CPUAddress;
			CPUAddress += offset;

			uint64_t GPUAddress = cBufferMap.GPUAddress;
			GPUAddress += offset;

			memcpy(CPUAddress, &m_constantBufferData, sizeof(m_constantBufferData));

			m_commandList->SetGraphicsRootConstantBufferView(0, GPUAddress);
		}


		// Record commands
		const eastl::vector<TransformObjPtr>& children = currModel->GetChildren();
		for (const TransformObjPtr& child : children)
		{
			const TransformObject* childPtr = child.get();

			if (const MeshNode* modelChild = dynamic_cast<const MeshNode*>(childPtr))
			{
				ID3D12DescriptorHeap* ppHeaps[] = { D3D12Globals::GlobalSRVHeap.Heap };
				m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

				// Set root to index of first texture
				m_commandList->SetGraphicsRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(modelChild->Textures[0]->SRVIndex));

				m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				m_commandList->IASetVertexBuffers(0, 1, &modelChild->VertexBuffer->VBView());
				m_commandList->IASetIndexBuffer(&modelChild->IndexBuffer->IBView());

				m_commandList->DrawIndexedInstanced(modelChild->IndexBuffer->IndexCount, 1, 0, 0, 0);
			}
		}
	}



}

void AppModeBase::EndFrame()
{
	//Draw ImGui
	ImGui::EndFrame();
	ImGui::Render();

	ImGuiRenderDrawData();

	SwapBuffers();
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

#if FRAME_BUFFERING
void AppModeBase::WaitForPreviousFrame()
{
	const UINT64 currentFrameRequiredFenceValue = m_fenceValues[D3D12Globals::CurrentFrameIndex];
	// We want that fence to be set to that value from the GPU side
	DXAssert(D3D12Globals::CommandQueue->Signal(m_fence.Get(), m_fenceValues[D3D12Globals::CurrentFrameIndex]));

	// Tell m_fence to raise this event once it's equal fence value
	DXAssert(m_fence->SetEventOnCompletion(m_fenceValues[D3D12Globals::CurrentFrameIndex], m_fenceEvent));

	// Wait until that event is raised
	WaitForSingleObject(m_fenceEvent, INFINITE);

	m_fenceValues[D3D12Globals::CurrentFrameIndex]++;
}
#else
void AppModeBase::WaitForPreviousFrame()
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
	DXAssert(D3D12Globals::CommandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	const UINT64 fenceValue = m_fence->GetCompletedValue();
	if (fenceValue < fence)
	{
		// Tell m_fence to raise this event once it's equal fence value
		DXAssert(m_fence->SetEventOnCompletion(fence, m_fenceEvent));

		// Wait until that event is raised
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	D3D12Globals::CurrentFrameIndex = D3D12Globals::SwapChain->GetCurrentBackBufferIndex();

	// Clean all used up upload buffers
	D3D12RHI::Get()->DoTextureUploadHack();
}
#endif


#if FRAME_BUFFERING
// Prepare to render the next frame.
void AppModeBase::MoveToNextFrame()
{
	const UINT64 submittedFrameFenceValue = m_fenceValues[D3D12Globals::CurrentFrameIndex];

	// Place signal for frame that was just submitted
	DXAssert(D3D12Globals::CommandQueue->Signal(m_fence.Get(), submittedFrameFenceValue));

	// Move onto next frame. Backbuffer index was changed as Present was called before this
	D3D12Globals::CurrentFrameIndex = D3D12Globals::SwapChain->GetCurrentBackBufferIndex();

	const UINT64 presentFenceValue = m_fence->GetCompletedValue();

	// Happens if Graphics device is removed while running
	ASSERT(presentFenceValue != UINT64_MAX);

	const UINT64 toStartFrameFenceValue = m_fenceValues[D3D12Globals::CurrentFrameIndex];

	if (presentFenceValue < toStartFrameFenceValue)
	{
		DXAssert(m_fence->SetEventOnCompletion(toStartFrameFenceValue, m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, false);
	}

	m_fenceValues[D3D12Globals::CurrentFrameIndex] = submittedFrameFenceValue + 1;
}
#endif


ComPtr<ID3D12DescriptorHeap> m_imguiCbvSrvHeap;

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

	bool success = ImGui_ImplDX12_Init(D3D12Globals::Device, D3D12Globals::NumFramesInFlight, format, m_imguiCbvSrvHeap.Get(), fontSrvCpuHandle, fontSrvGpuHandle);

	ASSERT(success);

}

void AppModeBase::ImGuiRenderDrawData()
{
	// Set the imgui descriptor heap
	ID3D12DescriptorHeap* imguiHeaps[] = { m_imguiCbvSrvHeap.Get() };
	m_commandList->SetDescriptorHeaps(1, imguiHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
}
