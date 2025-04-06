#include "BindlessDecalsPass.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "DeferredBasePass.h"
#include "Renderer/Drawable/Drawable.h"
#include "Renderer/RHI/D3D12/D3D12GraphicsTypes_Internal.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"
#include "Window/WindowsWindow.h"
#include "Core/AppCore.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "imgui.h"
#include <d3d12.h>
#include "Math/MathUtils.h"


class DecalObj : public DrawableObject
{
public:
	DecalObj(const eastl::string& inName)
		: DrawableObject(inName) {
	}
};

eastl::vector<eastl::shared_ptr<DecalObj>> SceneDecals;

D3D12StructuredBuffer m_DecalsBuffer;
D3D12RawBuffer m_DecalsTiledBinningBuffer;
eastl::shared_ptr<D3D12RenderTarget2D> m_DebugRT;

glm::vec<2, uint32_t> TileComputeGroupCounts;

// TODO: Send this to the shaders through the compiler defines
#define TILE_SIZE 16


ID3D12RootSignature* m_DecalRootSignature;
ID3D12RootSignature* m_TileBinningRootSignature;

ID3D12PipelineState* m_DecalPipelineState;
ID3D12PipelineState* m_TiledBinningPipelineState;


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


struct ShaderDecal
{
	glm::vec4 Orientation;	// 16 bytes
	glm::vec3 Size;		// 28 bytes
	glm::vec3 Position;	// 40 bytes
	uint32_t AlbedoMapIdx;	// 44 bytes
	uint32_t NormalMapIdx;	// 48 bytes
};

static_assert((sizeof(ShaderDecal) % 16) == 0, "Structs in Structured Buffers have to be 16-byte aligned");

BindlessDecalsPass::BindlessDecalsPass() = default;
BindlessDecalsPass::~BindlessDecalsPass() = default;

void BindlessDecalsPass::Init()
{
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



	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	m_DebugRT = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"DebugRT", ERHITexturePrecision::Float32, ETextureState::Shader_Resource, ERHITextureFilter::Nearest);


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



	m_DecalsBuffer.Init(1024, sizeof(ShaderDecal));

	constexpr uint64_t DecalElementsPerTile = 1;
	m_DecalsTiledBinningBuffer.Init(TileComputeGroupCounts.x * TileComputeGroupCounts.y * DecalElementsPerTile);

	SceneManager& sManager = SceneManager::Get();
	Scene& currentScene = sManager.GetCurrentScene();

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
}

void BindlessDecalsPass::ComputeTiledBinning(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures)
{
	PIXMarker Marker(inCmdList, "Tiled Binning");

	inCmdList->SetComputeRootSignature(m_TileBinningRootSignature);
	inCmdList->SetPipelineState(m_TiledBinningPipelineState);

	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	// 0. Structured Buffer
	// 1. Depth Buffer
	// 2. Constant Buffer
	// 3. Output UAV

	// Clear decal binning buffer
	{
		D3D12Utility::UAVBarrier(inCmdList, m_DecalsTiledBinningBuffer.Resource);

		const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHandles = { m_DecalsTiledBinningBuffer.UAV, m_DebugRT->UAV };
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = D3D12Utility::CreateTempDescriptorTable(inCmdList, uavHandles);

		uint32_t values[4] = {};
		inCmdList->ClearUnorderedAccessViewUint(gpuHandle, uavHandles[0], m_DecalsTiledBinningBuffer.Resource, values, 0, nullptr);

		// float floatValues[4] = {};
		// inCmdList->ClearRenderTargetView(m_DebugRT->RTV, floatValues, 0, nullptr);

		// inCmdList->ClearUnorderedAccessViewFloat(gpuHandle, uavHandles[1], m_DebugRT->Texture->Resource, floatValues, 0, nullptr);

	}

	D3D12Utility::UAVBarrier(inCmdList, m_DecalsTiledBinningBuffer.Resource);
	D3D12Utility::UAVBarrier(inCmdList, m_DebugRT->Texture->Resource);

	inCmdList->SetComputeRootShaderResourceView(0, m_DecalsBuffer.GetCurrentGPUAddress());
	inCmdList->SetComputeRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

	// TODO: Separate binning tile size and count from decal computing tile size and count

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	// 	glm::vec<2, uint32_t> binningComputeGroupCounts = glm::vec<2, uint32_t>(
	// 	MathUtils::DivideAndRoundUp(props.Width, 64),
	// 	MathUtils::DivideAndRoundUp(props.Height, 64));

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
		ImGui::DragFloat3("Debug Rot", &debugRot.x, 0.05f, -360.f, 360.f);

		const glm::quat q = glm::quat(glm::radians(debugRot));

		tilingConstBufferData.DebugQuat = glm::vec4(q.x, q.y, q.z, q.w);;

		tilingConstBufferData.DebugValue = glm::vec4(debugValue, 0.f, 0.f, 0.f);
		tilingConstBufferData.DebugFlag = uint32_t(debugFlag);
		tilingConstBufferData.NumDecals = SceneDecals.size();
		tilingConstBufferData.NumWorkGroups = TileComputeGroupCounts;

		// Use temp buffer in main constant buffer
		MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(tilingConstBufferData));
		memcpy(cBufferMap.CPUAddress, &tilingConstBufferData, sizeof(tilingConstBufferData));
		inCmdList->SetComputeRootConstantBufferView(2, cBufferMap.GPUAddress);
	}

	inCmdList->SetComputeRootUnorderedAccessView(3, m_DecalsTiledBinningBuffer.GetGPUAddress());

	const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHandles = { m_DebugRT->UAV };
	D3D12Utility::BindTempDescriptorTable(4, inCmdList, uavHandles);

	inCmdList->Dispatch(TileComputeGroupCounts.x, TileComputeGroupCounts.y, 1);
}


void BindlessDecalsPass::UpdateBeforeExecute()
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

void BindlessDecalsPass::ComputeDecals(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures)
{
	PIXMarker Marker(inCmdList, "Compute Decals");

	// Draw screen quad
	inCmdList->SetComputeRootSignature(m_DecalRootSignature);
	inCmdList->SetPipelineState(m_DecalPipelineState);

	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	const eastl::shared_ptr<Camera>& currentCamera = currentScene.GetCurrentCamera();

	// 0. Main CBV_SRV_UAV heap
	// 1. Structured Buffer
	// 2. Depth Buffer
	// 3. Constant Buffer
	// 5. Output UAV

	D3D12Utility::UAVBarrier(inCmdList, m_DecalsTiledBinningBuffer.Resource);

	inCmdList->SetComputeRootDescriptorTable(0, D3D12Globals::GlobalSRVHeap.GPUStart[D3D12Utility::CurrentFrameIndex]);
	inCmdList->SetComputeRootShaderResourceView(1, m_DecalsBuffer.GetCurrentGPUAddress());
	inCmdList->SetComputeRootShaderResourceView(2, m_DecalsTiledBinningBuffer.GetGPUAddress());
	inCmdList->SetComputeRootDescriptorTable(3, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

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
		inCmdList->SetComputeRootConstantBufferView(4, cBufferMap.GPUAddress);
	}

	const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> uavHandles = { inSceneTextures.GBufferAlbedo->UAV, inSceneTextures.GBufferNormal->UAV, inSceneTextures.GBufferRoughness->UAV };
	D3D12Utility::BindTempDescriptorTable(5, inCmdList, uavHandles);

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	const glm::vec<2, uint32_t> GroupCounts = glm::vec<2, uint32_t>(
		MathUtils::DivideAndRoundUp(props.Width, TILE_SIZE),
		MathUtils::DivideAndRoundUp(props.Height, TILE_SIZE));

	inCmdList->Dispatch(GroupCounts.x, GroupCounts.y, 1);
}





void BindlessDecalsPass::Execute(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures)
{
	{
		D3D12Utility::TransitionResource(inCmdList, m_DecalsTiledBinningBuffer.Resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Utility::TransitionResource(inCmdList, inSceneTextures.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		ComputeTiledBinning(inCmdList, inSceneTextures);

		D3D12Utility::TransitionResource(inCmdList, inSceneTextures.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		D3D12Utility::TransitionResource(inCmdList, m_DecalsTiledBinningBuffer.Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}



	{
		D3D12Utility::TransitionResource(inCmdList, inSceneTextures.GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Utility::TransitionResource(inCmdList, inSceneTextures.GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Utility::TransitionResource(inCmdList, inSceneTextures.GBufferRoughness->Texture->Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Utility::TransitionResource(inCmdList, inSceneTextures.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		ComputeDecals(inCmdList, inSceneTextures);
	}


}
