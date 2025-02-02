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

eastl::shared_ptr<D3D12IndexBuffer> ScreenQuadIndexBuffer = nullptr;
eastl::shared_ptr<D3D12VertexBuffer> ScreenQuadVertexBuffer = nullptr;

ID3D12Resource* m_BackBuffers[D3D12Utility::NumFramesInFlight];
eastl::shared_ptr<D3D12RenderTarget2D> m_DebugRT;

ID3D12CommandAllocator* m_commandAllocators[D3D12Utility::NumFramesInFlight];

ID3D12GraphicsCommandList* m_commandList;


ID3D12RootSignature* m_LightingRootSignature;
ID3D12RootSignature* m_DecalRootSignature;
ID3D12RootSignature* m_TileBinningRootSignature;


ID3D12PipelineState* m_LightingPipelineState;
ID3D12PipelineState* m_DecalPipelineState;
ID3D12PipelineState* m_TiledBinningPipelineState;

struct ShaderMaterial
{
	uint32_t AlbedoMapIndex;
	uint32_t NormalMapIndex;
	uint32_t MRMapIndex;
};

struct ShaderDecal
{
	glm::vec4 Orientation;	// 16 bytes
	glm::vec3 Size;		// 28 bytes
	glm::vec3 Position;	// 40 bytes
	uint32_t AlbedoMapIdx;	// 44 bytes
	uint32_t NormalMapIdx;	// 48 bytes
};

static_assert((sizeof(ShaderDecal) % 16) == 0, "Structs in Structured Buffers have to be 16-byte aligned");

struct LightingConstantBuffer
{
	glm::mat4 ViewInv;
	glm::mat4 ProjInv;
	glm::mat4 Proj;
	glm::vec4 ViewPos;
	glm::vec4 LightDir;

	float Padding[8];
};
static_assert((sizeof(LightingConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

struct DecalConstantBuffer
{
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 InvViewProj;
	uint32_t NumDecals;
	glm::vec<2, uint32_t> NumWorkGroups;

	float Padding[13];
};
static_assert((sizeof(DecalConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");


struct DecalTilingConstantBuffer
{
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 InvViewProj;
	uint32_t NumDecals;
	glm::vec<2, uint32_t> NumWorkGroups;
	uint32_t DebugFlag;
	glm::vec4 DebugValue;
	glm::vec4 DebugQuat;

	float Padding[4];
};
static_assert((sizeof(DecalTilingConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

D3D12StructuredBuffer m_DecalsBuffer;
D3D12RawBuffer m_DecalsTiledBinningBuffer;

class DecalObj : public DrawableObject
{
public:
	DecalObj(const eastl::string& inName)
		: DrawableObject(inName) {}
};
eastl::vector<eastl::shared_ptr<DecalObj>> SceneDecals;

DeferredBasePass DeferredBasePassCommand;

void AppModeBase::Init()
{
	BENCH_SCOPE("App Mode Init");


	D3D12RHI::Init();

	ImGuiInit();

	CreateInitialResources();
}

glm::vec<2, uint32_t> TileComputeGroupCounts;

// TODO: Send this to the shaders through the compiler defines
#define TILE_SIZE 16


void AppModeBase::CreateInitialResources()
{
	BENCH_SCOPE("Create Resources");

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	// The main idea of this binning is to combine MJPs bit representation of the binning result with the Forward+ algorithm for tiling only in 2D.

	// The tiled binning pass uses each thread in each tile to process one decal.
	// All threads in all tiles go over the same decals at the same time(theoretically), which means that if there are not 
	// many decals, a lot of the threads in each tile will just sleep.
	// There can be no more decals to process than the bit representation allows in the binned buffer, because each bit represents a decal idx which will be used to index in the same buffer used for processing.
	// This means that there is no reason for tiles to contain more threads than that representation allows decals, only less and they can go through them with a for.

	// The smaller the tile is, the more tiles will be launched.
	// To fit 256 decals in the scene, 256 bits are requires and thus 8 uint32 necessary. This means that the decal pass needs to go through a for with 8 elements.
	// 8 uint32s = 32 bytes per tile.
	// The number of tiles needs to be equal in the binning pass and the decal pass. At the end of the day, each decal tile needs results for one corresponding tile.
	// Again, the number of possible decals is completely separate from the number of tiles.
	// The number of possible decals is represented only by the number of uint32s per tile used in the binned buffer.
	// Tiles there can be as many as required. There should be a kind of correlation, though, as allowing many decals in the scene at once, they will probably be processed better by larger tiles(maybe).

	// 256 possible decals -> 32 bytes per tile. If we use 16 * 16 tiles, then we will have a thread for each possible decal in the decal scene buffer, not needing a for(it can still be present).
	// For 1920*1080, 1920/16 = 120, 1080/16 = ~68 => 120 * 68 tiles = 8160 tiles
	// 8160 tiles * 32 bytes = 261120 bytes = 0.27 megabytes, could probably increase size to have more decals.
	// Number of decals doesn't have to correlate to number of tiles, though. This can be done with a for depending on the number of uint32s, like in MJPs decal implementation.
	// At processing time, a for can be used as well to decouple the number of tiles from the number of decals.


	TileComputeGroupCounts = glm::vec<2, uint32_t>(
		MathUtils::DivideAndRoundUp(props.Width, TILE_SIZE),
		MathUtils::DivideAndRoundUp(props.Height, TILE_SIZE));


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

	m_DebugRT= D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"DebugRT", ERHITexturePrecision::Float32, ETextureState::Shader_Resource, ERHITextureFilter::Nearest);

	CreateRootSignatures();

	// Prepare Data

	CreatePSOs();
	
	// Create the command list.
	DXAssert(D3D12Globals::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[D3D12Utility::CurrentFrameIndex], nullptr, IID_PPV_ARGS(&m_commandList)));
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

	D3D12Globals::GlobalMaterialsBuffer.Init(1024, sizeof(ShaderMaterial));
	m_DecalsBuffer.Init(1024, sizeof(ShaderDecal));

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

	{
		eastl::shared_ptr<DecalObj> decalObj = eastl::make_shared<DecalObj>("Decal");
		currentScene.AddObject(decalObj);
		SceneDecals.push_back(decalObj);

		decalObj->SetRelativeLocation(glm::vec3(0.f, -1.f, 0.f));
		decalObj->SetRotationDegrees(glm::vec3(90.f, 0.f, 0.f));
	}

	//{
	//	eastl::shared_ptr<DecalObj> decalObj = eastl::make_shared<DecalObj>("Decal");
	//	currentScene.AddObject(decalObj);
	//	SceneDecals.push_back(decalObj);

	//	decalObj->SetRelativeLocation(glm::vec3(0.f, -1.f, -5.f));
	//	decalObj->SetRotationDegrees(glm::vec3(90.f, 0.f, 0.f));
	//}


	{
		eastl::vector<ShaderDecal> shaderDecals;
		for (uint32_t i = 0; i < SceneDecals.size(); ++i)
		{
			ShaderDecal newDecal = {};
			const Transform& absTrans = SceneDecals[i]->GetAbsoluteTransform();

			newDecal.Orientation = glm::vec4(absTrans.Rotation.x, absTrans.Rotation.y, absTrans.Rotation.z, absTrans.Rotation.w);
			newDecal.Position = absTrans.Translation;
			newDecal.Size = absTrans.Scale;

			newDecal.AlbedoMapIdx = 13;
			newDecal.NormalMapIdx = 21;

			shaderDecals.push_back(newDecal);
		}

		m_DecalsBuffer.UploadDataAllFrames(&shaderDecals[0], sizeof(ShaderDecal) * shaderDecals.size());
	}

	constexpr uint64_t DecalElementsPerTile = 1;
	m_DecalsTiledBinningBuffer.Init(TileComputeGroupCounts.x * TileComputeGroupCounts.y * DecalElementsPerTile);

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


void AppModeBase::CreateRootSignatures()
{

	// Final lighting root signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[3] = {};

		// Constant Buffer
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// Textures
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_DESCRIPTOR_RANGE1 texturesRange[1];
		texturesRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		texturesRange[0].BaseShaderRegister = 0;
		texturesRange[0].RegisterSpace = 0;
		texturesRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		texturesRange[0].OffsetInDescriptorsFromTableStart = 0;

		// GBuffer Albedo, GBuffer Normal and GBuffer Roughness
		texturesRange[0].NumDescriptors = 3;

		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(texturesRange);
		rootParameters[1].DescriptorTable.pDescriptorRanges = &texturesRange[0];

		// Depth Buffer
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_DESCRIPTOR_RANGE1 depthBufferRange[1];
		depthBufferRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		depthBufferRange[0].BaseShaderRegister = 3;
		depthBufferRange[0].RegisterSpace = 0;
		depthBufferRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		depthBufferRange[0].OffsetInDescriptorsFromTableStart = 0;
		depthBufferRange[0].NumDescriptors = 1;

		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(depthBufferRange);
		rootParameters[2].DescriptorTable.pDescriptorRanges = &depthBufferRange[0];

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

	// Decal Pass Signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[6];

		// 0. Main CBV_SRV_UAV heap
		// 1. Structured Buffer
		// 2. Depth Buffer
		// 3. Constant Buffer
		// 4. Root Constant
		// 5. Output UAV

		// Main CBV_SRV_UAV heap
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_DESCRIPTOR_RANGE1 srvRangeCS[1] = {};
		srvRangeCS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRangeCS[0].NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		srvRangeCS[0].BaseShaderRegister = 0;
		srvRangeCS[0].RegisterSpace = 0;
		srvRangeCS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		srvRangeCS[0].OffsetInDescriptorsFromTableStart = 0;

		rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRangeCS[0];
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(srvRangeCS);

		// Structured Buffer
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameters[1].Descriptor.RegisterSpace = 100;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		// Binning Buffer
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[2].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameters[2].Descriptor.RegisterSpace = 100;
		rootParameters[2].Descriptor.ShaderRegister = 1;

		// Depth Buffer
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_DESCRIPTOR_RANGE1 depthBufferRange[1];
		depthBufferRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		depthBufferRange[0].BaseShaderRegister = 2;
		depthBufferRange[0].RegisterSpace = 100;
		depthBufferRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		depthBufferRange[0].OffsetInDescriptorsFromTableStart = 0;
		depthBufferRange[0].NumDescriptors = 1;

		rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(depthBufferRange);
		rootParameters[3].DescriptorTable.pDescriptorRanges = &depthBufferRange[0];

		// Constant Buffer
		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[4].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[4].Descriptor.RegisterSpace = 0;
		rootParameters[4].Descriptor.ShaderRegister = 0;

		// Output UAV
		D3D12_DESCRIPTOR_RANGE1 uavRangeCS[1] = {};
		uavRangeCS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRangeCS[0].NumDescriptors = 3;
		uavRangeCS[0].BaseShaderRegister = 0;
		uavRangeCS[0].RegisterSpace = 0;
		uavRangeCS[0].OffsetInDescriptorsFromTableStart = 0;

		rootParameters[5].DescriptorTable.pDescriptorRanges = &uavRangeCS[0];
		rootParameters[5].DescriptorTable.NumDescriptorRanges = _countof(uavRangeCS);
		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		//////////////////////////////////////////////////////////////////////////

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_ANISOTROPIC;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 16;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = &rootParameters[0];
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags = rootSignatureFlags;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
		versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

		m_DecalRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
	}

	// Tiled Binning Root Signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[5];

		// 0. Structured Buffer
		// 1. Depth Buffer
		// 2. Constant Buffer
		// 3. Output UAV
		// 4. Debug UAV

		// Structured Buffer
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// Depth Buffer
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_DESCRIPTOR_RANGE1 depthBufferRange[1];
		depthBufferRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		depthBufferRange[0].BaseShaderRegister = 1;
		depthBufferRange[0].RegisterSpace = 0;
		depthBufferRange[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		depthBufferRange[0].OffsetInDescriptorsFromTableStart = 0;
		depthBufferRange[0].NumDescriptors = 1;

		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(depthBufferRange);
		rootParameters[1].DescriptorTable.pDescriptorRanges = &depthBufferRange[0];

		// Constant Buffer
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[2].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[2].Descriptor.RegisterSpace = 0;
		rootParameters[2].Descriptor.ShaderRegister = 0;

		// Output UAV
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[3].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameters[3].Descriptor.RegisterSpace = 0;
		rootParameters[3].Descriptor.ShaderRegister = 0;

		// Debug UAV
		D3D12_DESCRIPTOR_RANGE1 uavRangeCS[1] = {};
		uavRangeCS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRangeCS[0].NumDescriptors = 1;
		uavRangeCS[0].BaseShaderRegister = 1;
		uavRangeCS[0].RegisterSpace = 0;
		uavRangeCS[0].OffsetInDescriptorsFromTableStart = 0;

		rootParameters[4].DescriptorTable.pDescriptorRanges = &uavRangeCS[0];
		rootParameters[4].DescriptorTable.NumDescriptorRanges = _countof(uavRangeCS);
		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		//////////////////////////////////////////////////////////////////////////

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_ANISOTROPIC;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 16;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = &rootParameters[0];
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags = rootSignatureFlags;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
		versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

		m_TileBinningRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
	}


}

void AppModeBase::CreatePSOs()
{

	// Lighting Quad PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "LightingPass.hlsl";

		CompiledShaderResult meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath);

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

	// Compute Decal Pass PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/";
		fullPath += "DecalPass.hlsl";

		const CompiledShaderResult compiledShader = D3D12RHI::Get()->CompileComputeShaderFromFile(fullPath);

		// shader bytecodes
		D3D12_SHADER_BYTECODE csByteCode;
		csByteCode.pShaderBytecode = compiledShader.CSByteCode->GetBufferPointer();
		csByteCode.BytecodeLength = compiledShader.CSByteCode->GetBufferSize();

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_DecalRootSignature;
		psoDesc.CS = csByteCode;

		DXAssert(D3D12Globals::Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_DecalPipelineState)));
	}

	// Compute Decal Pass PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/";
		fullPath += "TiledBinning.hlsl";

		const CompiledShaderResult compiledShader = D3D12RHI::Get()->CompileComputeShaderFromFile(fullPath);

		// shader bytecodes
		D3D12_SHADER_BYTECODE csByteCode;
		csByteCode.pShaderBytecode = compiledShader.CSByteCode->GetBufferPointer();
		csByteCode.BytecodeLength = compiledShader.CSByteCode->GetBufferSize();

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_TileBinningRootSignature;
		psoDesc.CS = csByteCode;

		DXAssert(D3D12Globals::Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_TiledBinningPipelineState)));
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

	{
		eastl::vector<ShaderDecal> shaderDecals;
		for (uint32_t i = 0; i < SceneDecals.size(); ++i)
		{
			ShaderDecal newDecal = {};
			const Transform& absTrans = SceneDecals[i]->GetAbsoluteTransform();

			newDecal.Orientation = glm::vec4(absTrans.Rotation.x, absTrans.Rotation.y, absTrans.Rotation.z, absTrans.Rotation.w);
			newDecal.Position = absTrans.Translation;
			newDecal.Size = absTrans.Scale;

			newDecal.AlbedoMapIdx = 13;
			newDecal.NormalMapIdx = 21;

			shaderDecals.push_back(newDecal);
		}

		m_DecalsBuffer.UploadDataCurrentFrame(&shaderDecals[0], sizeof(ShaderDecal) * shaderDecals.size());
	}

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


void AppModeBase::RenderLighting(SceneTextures& inSceneTextures)
{
	PIXMarker Marker(m_commandList, "Render Deferred Lighting");

	// Draw screen quad

	// Backbuffers are the first 2 RTVs in the Global Heap
	D3D12_CPU_DESCRIPTOR_HANDLE currentBackbufferRTDescriptor = D3D12Globals::GlobalRTVHeap.GetCPUHandle(D3D12Utility::CurrentFrameIndex, 0);
	m_commandList->ClearRenderTargetView(currentBackbufferRTDescriptor, D3D12Utility::ClearColor, 0, nullptr);

	m_commandList->SetGraphicsRootSignature(m_LightingRootSignature);
	m_commandList->SetPipelineState(m_LightingPipelineState);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
	renderTargets[0] = currentBackbufferRTDescriptor;

	m_commandList->OMSetRenderTargets(1, renderTargets, FALSE, nullptr);

	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();
	const eastl::shared_ptr<Camera>& currentCamera = currentScene.GetCurrentCamera();

	static glm::vec3 LightDir = glm::vec3(1.f, -1.f, 0.f);
	ImGui::DragFloat3("Light Direction", &LightDir.x, 0.05f, -1.f, 1.f);

	{
		LightingConstantBuffer lightingConstantBufferData;

		lightingConstantBufferData.ProjInv = glm::transpose(glm::inverse(currentScene.GetMainCameraProj()));
		lightingConstantBufferData.ViewInv = glm::transpose(glm::inverse(currentScene.GetMainCameraLookAt()));
		lightingConstantBufferData.Proj = glm::transpose(currentScene.GetMainCameraProj());

		lightingConstantBufferData.LightDir = glm::vec4(glm::normalize(LightDir), 0.f);
		lightingConstantBufferData.ViewPos = glm::vec4(currentCamera->GetAbsoluteTransform().Translation, 0.f);

		// Use temp buffer in main constant buffer
		MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(lightingConstantBufferData));
		memcpy(cBufferMap.CPUAddress, &lightingConstantBufferData, sizeof(lightingConstantBufferData));
		m_commandList->SetGraphicsRootConstantBufferView(0, cBufferMap.GPUAddress);
	}

	m_commandList->SetGraphicsRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.GBufferAlbedo->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));


	m_commandList->SetGraphicsRootDescriptorTable(2, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const D3D12_VERTEX_BUFFER_VIEW vbView = ScreenQuadVertexBuffer->VBView();
	const D3D12_INDEX_BUFFER_VIEW ibView = ScreenQuadIndexBuffer->IBView();
	m_commandList->IASetVertexBuffers(0, 1, &vbView);
	m_commandList->IASetIndexBuffer(&ibView);

	m_commandList->DrawIndexedInstanced(ScreenQuadIndexBuffer->IndexCount, 1, 0, 0, 0);


}

void AppModeBase::ComputeTiledBinning(SceneTextures& inSceneTextures)
{
	PIXMarker Marker(m_commandList, "Tiled Binning");

	m_commandList->SetComputeRootSignature(m_TileBinningRootSignature);
	m_commandList->SetPipelineState(m_TiledBinningPipelineState);

	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	// 0. Structured Buffer
	// 1. Depth Buffer
	// 2. Constant Buffer
	// 3. Output UAV

	// Clear decal binning buffer
	{
		D3D12Utility::UAVBarrier(m_commandList, m_DecalsTiledBinningBuffer.Resource);

		const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHandles = { m_DecalsTiledBinningBuffer.UAV, m_DebugRT->UAV };
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = D3D12Utility::CreateTempDescriptorTable(m_commandList, uavHandles);

		uint32_t values[4] = {};
		m_commandList->ClearUnorderedAccessViewUint(gpuHandle, uavHandles[0], m_DecalsTiledBinningBuffer.Resource, values, 0, nullptr);

// 		float floatValues[4] = {};
// 		m_commandList->ClearRenderTargetView(m_DebugRT->RTV, floatValues, 0, nullptr);

		//m_commandList->ClearUnorderedAccessViewFloat(gpuHandle, uavHandles[1], m_DebugRT->Texture->Resource, floatValues, 0, nullptr);

	}

	D3D12Utility::UAVBarrier(m_commandList, m_DecalsTiledBinningBuffer.Resource);
	D3D12Utility::UAVBarrier(m_commandList, m_DebugRT->Texture->Resource);

	m_commandList->SetComputeRootShaderResourceView(0, m_DecalsBuffer.GetCurrentGPUAddress());
	m_commandList->SetComputeRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

	// TODO: Separate binning tile size and count from decal computing tile size and count

 	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
 	const WindowProperties& props = mainWindow.GetProperties();
// 
// 	glm::vec<2, uint32_t> binningComputeGroupCounts = glm::vec<2, uint32_t>(
// 		MathUtils::DivideAndRoundUp(props.Width, 64),
// 		MathUtils::DivideAndRoundUp(props.Height, 64));

	{
		DecalTilingConstantBuffer tilingConstBufferData;

		tilingConstBufferData.Projection = glm::transpose(currentScene.GetMainCameraProj());
		tilingConstBufferData.View = glm::transpose(currentScene.GetMainCameraLookAt());
		tilingConstBufferData.InvViewProj = glm::transpose(glm::inverse(currentScene.GetMainCameraProj() * currentScene.GetMainCameraLookAt()));

		static int32_t debugFlag = 1;
		static float debugValue = 1.f;
		static glm::vec3 debugRot; // Degrees


		ImGui::SliderInt("Debug Flag", &debugFlag, 0, 1);
		ImGui::SliderFloat("Debug Value", &debugValue, -1.f, 1.f);
		ImGui::DragFloat3("Debug Rot", &debugRot.x, 0.05f,-360.f, 360.f);

		const glm::quat q = glm::quat(glm::radians(debugRot));
		
		tilingConstBufferData.DebugQuat = glm::vec4(q.x, q.y, q.z, q.w);;
		
		tilingConstBufferData.DebugValue = glm::vec4(debugValue, 0.f, 0.f, 0.f);
		tilingConstBufferData.DebugFlag = uint32_t(debugFlag);
		tilingConstBufferData.NumDecals = SceneDecals.size();
		tilingConstBufferData.NumWorkGroups = TileComputeGroupCounts;

		// Use temp buffer in main constant buffer
		MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(tilingConstBufferData));
		memcpy(cBufferMap.CPUAddress, &tilingConstBufferData, sizeof(tilingConstBufferData));
		m_commandList->SetComputeRootConstantBufferView(2, cBufferMap.GPUAddress);
	}

	m_commandList->SetComputeRootUnorderedAccessView(3, m_DecalsTiledBinningBuffer.GetGPUAddress());

	const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHandles = { m_DebugRT->UAV};
	D3D12Utility::BindTempDescriptorTable(4, m_commandList, uavHandles);

	m_commandList->Dispatch(TileComputeGroupCounts.x, TileComputeGroupCounts.y, 1);
}

void AppModeBase::ComputeDecals(SceneTextures& inSceneTextures)
{
	PIXMarker Marker(m_commandList, "Compute Decals");

	// Draw screen quad
	m_commandList->SetComputeRootSignature(m_DecalRootSignature);
	m_commandList->SetPipelineState(m_DecalPipelineState);

	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	const eastl::shared_ptr<Camera>& currentCamera = currentScene.GetCurrentCamera();

	// 0. Main CBV_SRV_UAV heap
	// 1. Structured Buffer
	// 2. Depth Buffer
	// 3. Constant Buffer
	// 5. Output UAV

	D3D12Utility::UAVBarrier(m_commandList, m_DecalsTiledBinningBuffer.Resource);

	m_commandList->SetComputeRootDescriptorTable(0, D3D12Globals::GlobalSRVHeap.GPUStart[D3D12Utility::CurrentFrameIndex]);
	m_commandList->SetComputeRootShaderResourceView(1, m_DecalsBuffer.GetCurrentGPUAddress());
	m_commandList->SetComputeRootShaderResourceView(2, m_DecalsTiledBinningBuffer.GetGPUAddress());
	m_commandList->SetComputeRootDescriptorTable(3, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

	{
		DecalConstantBuffer decalConstantBufferData;

		decalConstantBufferData.Projection = glm::transpose(currentScene.GetMainCameraProj());
		decalConstantBufferData.View = glm::transpose(currentScene.GetMainCameraLookAt());
		decalConstantBufferData.InvViewProj = glm::transpose(glm::inverse(currentScene.GetMainCameraProj() * currentScene.GetMainCameraLookAt()));
		decalConstantBufferData.NumDecals = SceneDecals.size();
		decalConstantBufferData.NumWorkGroups = TileComputeGroupCounts;

		// Use temp buffer in main constant buffer
		MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(decalConstantBufferData));
		memcpy(cBufferMap.CPUAddress, &decalConstantBufferData, sizeof(decalConstantBufferData));
		m_commandList->SetComputeRootConstantBufferView(4, cBufferMap.GPUAddress);
	}

	const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHandles = { inSceneTextures.GBufferAlbedo->UAV, inSceneTextures.GBufferNormal->UAV, inSceneTextures.GBufferRoughness->UAV };
	D3D12Utility::BindTempDescriptorTable(5, m_commandList, uavHandles);

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	const glm::vec<2, uint32_t> GroupCounts = glm::vec<2, uint32_t>(
		MathUtils::DivideAndRoundUp(props.Width, TILE_SIZE),
		MathUtils::DivideAndRoundUp(props.Height, TILE_SIZE));

	m_commandList->Dispatch(GroupCounts.x, GroupCounts.y, 1);
}




void AppModeBase::Draw()
{
	static bool doOnce = false;
	if (!doOnce)
	{
		D3D12Utility::TransitionResource(m_commandList, D3D12Globals::GlobalMaterialsBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		doOnce = true;
	}

	{
		DeferredBasePassCommand.Execute(m_commandList);

	}

	//{
	//	D3D12Utility::TransitionResource(m_commandList, m_DecalsTiledBinningBuffer.Resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	D3D12Utility::TransitionResource(m_commandList, m_MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//	ComputeTiledBinning();

	//	D3D12Utility::TransitionResource(m_commandList, m_MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//	D3D12Utility::TransitionResource(m_commandList, m_DecalsTiledBinningBuffer.Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//}



	//{
	//	D3D12Utility::TransitionResource(m_commandList, m_GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	D3D12Utility::TransitionResource(m_commandList, m_GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	D3D12Utility::TransitionResource(m_commandList, m_GBufferRoughness->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	D3D12Utility::TransitionResource(m_commandList, m_MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//	ComputeDecals();
	//}


	{
		SceneTextures& sceneTextures = DeferredBasePassCommand.GBufferTextures;

		D3D12_CPU_DESCRIPTOR_HANDLE currentBackbufferRTDescriptor = D3D12Globals::GlobalRTVHeap.GetCPUHandle(D3D12Utility::CurrentFrameIndex, 0);
		D3D12Utility::TransitionResource(m_commandList, m_BackBuffers[D3D12Utility::CurrentFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		D3D12Utility::TransitionResource(m_commandList, sceneTextures.GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		D3D12Utility::TransitionResource(m_commandList, sceneTextures.GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		D3D12Utility::TransitionResource(m_commandList, sceneTextures.GBufferRoughness->Texture->Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		D3D12Utility::TransitionResource(m_commandList, sceneTextures.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		RenderLighting(sceneTextures);
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
