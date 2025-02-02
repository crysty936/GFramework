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
#include "Renderer/Model/3D/Assimp/AssimpModel3D.h"
#include "Math/MathUtils.h"
#include "glm/gtc/type_ptr.inl"
#include "Renderer/RenderPasses/DeferredBasePass.h"
#include "Renderer/RenderPasses/BindlessDecalsPass.h"
#include "Renderer/RenderPasses/DeferredLighting.h"

// Windows includes
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             
#endif
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
#include <wrl/client.h>

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
ID3D12Resource* m_BackBuffers[D3D12Utility::NumFramesInFlight];

ID3D12CommandAllocator* m_commandAllocators[D3D12Utility::NumFramesInFlight];

ID3D12GraphicsCommandList* m_commandList;

struct ShaderMaterial
{
	uint32_t AlbedoMapIndex;
	uint32_t NormalMapIndex;
	uint32_t MRMapIndex;
};

DeferredBasePass DeferredBasePassCommand;
BindlessDecalsPass BindlessDecalsPassCommmand;
DeferredLighting DeferredLightingCommand;

void AppModeBase::Init()
{
	BENCH_SCOPE("App Mode Init");


	D3D12RHI::Init();

	ImGuiInit();

	CreateInitialResources();
}


void AppModeBase::CreateInitialResources()
{
	BENCH_SCOPE("Create Resources");

	// TODO: Move this to RHI Init
	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		constexpr uint32_t numRTVs = 32;
		D3D12Globals::GlobalRTVHeap.Init(false, numRTVs, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		constexpr uint32_t numSRVs = 1024;
		constexpr uint32_t numTempSRVs = 128;
		D3D12Globals::GlobalSRVHeap.Init(true, numSRVs, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numTempSRVs);

		constexpr uint32_t numDSVs = 32;
		D3D12Globals::GlobalDSVHeap.Init(false, numDSVs, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		constexpr uint32_t numUAVs = 128;
		D3D12Globals::GlobalUAVHeap.Init(false, numUAVs, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		D3D12Globals::GlobalConstantsBuffer.Init(2 * 1024 * 1024);
	}

	// Create frame resources.
	{
		// Create a RTV for each frame.
		for (UINT i = 0; i < D3D12Utility::NumFramesInFlight; i++)
		{
			// Allocate descriptor space
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = D3D12Globals::GlobalRTVHeap.AllocatePersistent().CPUHandle[0];

			// Get a reference to the swapchain buffer
			DXAssert(D3D12Globals::SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));

			// Create the descriptor at the target location in the heap
			D3D12Globals::Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, cpuHandle);
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

	DeferredBasePassCommand.Init();
	BindlessDecalsPassCommmand.Init();
	DeferredLightingCommand.Init();

	// Create the command list.
	DXAssert(D3D12Globals::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[D3D12Utility::CurrentFrameIndex], nullptr, IID_PPV_ARGS(&m_commandList)));
	m_commandList->SetName(L"Main GFX Cmd List");

	// Memory upload test
	//for (int i = 0; i < 500; ++i)
	//{
	//	D3D12RHI::Get()->CreateAndLoadTexture2D("../Data/Textures/MinecraftGrass.jpg", /*inSRGB*/ true, m_commandList.Get());
	//}

	D3D12Globals::GlobalMaterialsBuffer.Init(1024, sizeof(ShaderMaterial));

	SceneManager& sManager = SceneManager::Get();
	Scene& currentScene = sManager.GetCurrentScene();

	// Cubes creation

	//eastl::shared_ptr<Model3D> TheCube = eastl::make_shared<CubeShape>("TheCube");
	//TheCube->Init(m_commandList);

	//eastl::shared_ptr<CubeShape> theCube2 = eastl::make_shared<CubeShape>("TheCube2");
	//theCube2->Init(m_commandList.Get());
	//TheCube->AddChild(theCube2);
	//theCube2->Move(glm::vec3(-5.f, 0.f, 0.f));

	//currentScene.AddObject(TheCube);

	//eastl::shared_ptr<TBNQuadShape> model = eastl::make_shared<TBNQuadShape>("TBN Quad");
	//model->Init(m_commandList);
	//currentScene.AddObject(model);


	//eastl::shared_ptr<AssimpModel3D> model= eastl::make_shared<AssimpModel3D>("../Data/Models/Sponza2/Sponza.fbx", "Sponza");
	eastl::shared_ptr<AssimpModel3D> model= eastl::make_shared<AssimpModel3D>("../Data/Models/Sponza/Sponza.gltf", "Sponza");
	model->Rotate(90.f, glm::vec3(0.f, 1.f, 0.f));
	model->Move(glm::vec3(0.f, -1.f, -5.f));
	model->Init(m_commandList);

	//eastl::shared_ptr<AssimpModel3D> model = eastl::make_shared<AssimpModel3D>("../Data/Models/Floor/scene.gltf", "Floor Model");
	//model->SetScale(glm::vec3(0.05f, 0.05f, 0.05f));
	//model->Move(glm::vec3(0.f, -3.f, 0.f));
	//model->Init(m_commandList);

	//eastl::shared_ptr<AssimpModel3D> model= eastl::make_shared<AssimpModel3D>("../Data/Models/Sphere/scene.gltf", "Sphere");
	//model->Init(m_commandList);

	//eastl::shared_ptr<AssimpModel3D> model= eastl::make_shared<AssimpModel3D>("../Data/Models/Shiba/scene.gltf", "Shiba");
	//model->Init(m_commandList);
	
	currentScene.AddObject(model);

	// Setup the materials for this model and upload to the mat buffer, hacky
	{
		eastl::vector<ShaderMaterial> shaderMats;
		for (const MeshMaterial& mat : model->Materials)
		{
			ShaderMaterial newShaderMat;
			newShaderMat.AlbedoMapIndex = mat.AlbedoMap != nullptr ? mat.AlbedoMap->SRVIndex : -1;
			newShaderMat.NormalMapIndex = mat.NormalMap != nullptr ? mat.NormalMap->SRVIndex : -1;
			newShaderMat.MRMapIndex = mat.MRMap != nullptr ? mat.MRMap->SRVIndex : -1;

			shaderMats.push_back(newShaderMat);
		}

		D3D12Globals::GlobalMaterialsBuffer.UploadDataAllFrames(&shaderMats[0], sizeof(ShaderMaterial) * shaderMats.size());
	}





	currentScene.GetCurrentCamera()->Move(EMovementDirection::Back, 10.f);

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


void AppModeBase::SwapBuffers()
{
	D3D12Utility::TransitionResource(m_commandList, m_BackBuffers[D3D12Utility::CurrentFrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

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

	D3D12Utility::CurrentFrameIndex = D3D12Globals::SwapChain->GetCurrentBackBufferIndex();

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
	DXAssert(m_commandAllocators[D3D12Utility::CurrentFrameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	DXAssert(m_commandList->Reset(m_commandAllocators[D3D12Utility::CurrentFrameIndex], nullptr));
}

void AppModeBase::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//D3D12RHI::Get()->BeginFrame();
	ResetFrameResources();

	// The descriptor heap needs to be set at least once per frame and preferrably never changed during that frame
	// as a change in descriptor heaps can incur a pipeline flush
	// Only one CBV SRV UAV heap and one Samplers heap can be bound at the same time
	ID3D12DescriptorHeap* ppHeaps[] = { D3D12Globals::GlobalSRVHeap.Heaps[D3D12Utility::CurrentFrameIndex] };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	BindlessDecalsPassCommmand.UpdateBeforeExecute();

	//ImGui::ShowDemoWindow();

	// Set viewport and scissor region
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
}



void AppModeBase::ExecutePasses()
{
	static bool doOnce = false;
	if (!doOnce)
	{
		D3D12Utility::TransitionResource(m_commandList, D3D12Globals::GlobalMaterialsBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		doOnce = true;
	}

	DeferredBasePassCommand.Execute(m_commandList);
	BindlessDecalsPassCommmand.Execute(m_commandList, DeferredBasePassCommand.GBufferTextures);


	{
		
		//D3D12_CPU_DESCRIPTOR_HANDLE currentBackbufferRTDescriptor = D3D12Globals::GlobalRTVHeap.GetCPUHandle(D3D12Utility::CurrentFrameIndex, 0);
		D3D12Utility::TransitionResource(m_commandList, m_BackBuffers[D3D12Utility::CurrentFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		SceneTextures& sceneTextures = DeferredBasePassCommand.GBufferTextures;

		DeferredLightingCommand.Execute(m_commandList, DeferredBasePassCommand.GBufferTextures);
	}

	// Draw scene hierarchy
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

	// Make sure that everything is synced up before executing command list
	// This also executes end frame on the Upload Queue, which (at the moment) forces a wait until everything is uploaded
	D3D12RHI::Get()->EndFrame();

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
	if (gpuLag >= D3D12Utility::NumFramesInFlight)
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

	bool success = ImGui_ImplDX12_Init(D3D12Globals::Device, D3D12Utility::NumFramesInFlight, format, m_imguiCbvSrvHeap, fontSrvCpuHandle, fontSrvGpuHandle);

	ASSERT(success);

}

void AppModeBase::ImGuiRenderDrawData()
{
	PIXMarker Marker(m_commandList, "Draw ImGui");

	// Set the imgui descriptor heap
	ID3D12DescriptorHeap* imguiHeaps[] = { m_imguiCbvSrvHeap };
	m_commandList->SetDescriptorHeaps(1, imguiHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);
}
