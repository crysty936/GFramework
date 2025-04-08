#include "DeferredBasePass.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "Renderer/RHI/Resources/RHITexture.h"
#include <d3d12.h>
#include "Window/WindowsWindow.h"
#include "Window/WindowProperties.h"
#include "Core/AppCore.h"
#include "Entity/TransformObject.h"
#include "Renderer/Model/3D/Model3D.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Renderer/RHI/D3D12/D3D12Utility.h"

// Constant Buffer
struct MeshConstantBuffer
{
	glm::mat4 LocalToClip;
	glm::mat4 LocalToWorldRotationOnly;
	uint32_t Padding[32];
};
static_assert((sizeof(MeshConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

ID3D12RootSignature* m_GBufferMainMeshRootSignature;
ID3D12RootSignature* m_GBufferBasicObjectsRootSignature;

ID3D12PipelineState* m_MainMeshPassPipelineState;
ID3D12PipelineState* m_BasicObjectsPipelineState;

void DeferredBasePass::Init()
{
	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	// Textures
	GBufferTextures.GBufferAlbedo = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"GBufferAlbedo", ERHITexturePrecision::UnsignedByte, ETextureState::Shader_Resource, ERHITextureFilter::Nearest);
	GBufferTextures.GBufferNormal = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"GBufferNormal", ERHITexturePrecision::Float32, ETextureState::Shader_Resource, ERHITextureFilter::Nearest);
	GBufferTextures.GBufferRoughness = D3D12RHI::Get()->CreateRenderTexture(props.Width, props.Height, L"GBufferRoughness", ERHITexturePrecision::UnsignedByte, ETextureState::Shader_Resource, ERHITextureFilter::Nearest);

	GBufferTextures.MainDepthBuffer = D3D12RHI::Get()->CreateDepthBuffer(props.Width, props.Height, L"Main Depth Buffer", ETextureState::Shader_Resource);



	// Root Signatures

	// GBuffer Main Mesh Pass signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[4];

		// Main CBV_SRV_UAV heap
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
		rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		rangesPS[0].NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rangesPS[0].BaseShaderRegister = 0;
		rangesPS[0].RegisterSpace = 0;
		rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

		rootParameters[0].DescriptorTable.pDescriptorRanges = &rangesPS[0];
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(rangesPS);

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameters[1].Descriptor.RegisterSpace = 100;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[2].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[2].Descriptor.RegisterSpace = 0;
		rootParameters[2].Descriptor.ShaderRegister = 0;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Constants.RegisterSpace = 0;
		rootParameters[3].Constants.ShaderRegister = 0;
		rootParameters[3].Constants.Num32BitValues = 1;


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
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

		m_GBufferMainMeshRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
	}


	// GBuffer Basic Shapes Pass signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[3];

		// Main CBV_SRV_UAV heap
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
		rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		rangesPS[0].NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rangesPS[0].BaseShaderRegister = 0;
		rangesPS[0].RegisterSpace = 0;
		rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

		rootParameters[0].DescriptorTable.pDescriptorRanges = &rangesPS[0];
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(rangesPS);

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].Constants.RegisterSpace = 0;
		rootParameters[2].Constants.ShaderRegister = 0;
		rootParameters[2].Constants.Num32BitValues = 1;



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
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = &rootParameters[0];
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags = rootSignatureFlags;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
		versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

		m_GBufferBasicObjectsRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
		m_GBufferBasicObjectsRootSignature->SetName(L"Basic Objects Root Signature");
	}


	// PSOs
	
	// Mesh Pass PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "MeshPass.hlsl";

		CompiledShaderResult meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath);

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
		psoDesc.NumRenderTargets = 3;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_MainMeshPassPipelineState)));
	}

	// Basic Shapes PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "BasicShapesMeshPass.hlsl";

		CompiledShaderResult meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath);

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
		psoDesc.pRootSignature = m_GBufferBasicObjectsRootSignature;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;
		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::BackFaceCull);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::WriteEnabled);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 3;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_BasicObjectsPipelineState)));
		m_BasicObjectsPipelineState->SetName(L"Basic Objects Pipeline State");

	}


}

static uint64_t TestNrMeshesToDraw = uint64_t(-1);
static uint64_t NrMeshesDrawn = 0;

static void DrawMeshNodesRecursively(ID3D12GraphicsCommandList* inCmdList, const eastl::vector<TransformObjPtr>& inChildNodes, const Scene& inCurrentScene, const eastl::vector<MeshMaterial>& inMaterials)
{
	for (const TransformObjPtr& child : inChildNodes)
	{
		const TransformObject* childPtr = child.get();

		DrawMeshNodesRecursively(inCmdList, childPtr->GetChildren(), inCurrentScene, inMaterials);

		if (NrMeshesDrawn >= TestNrMeshesToDraw)
		{
			return;
		}

		if (const MeshNode* modelChild = dynamic_cast<const MeshNode*>(childPtr))
		{
			if (modelChild->MatIndex == uint32_t(-1) || inMaterials.size() == 0)
			{
				continue;
			}

			const Transform& absTransform = modelChild->GetAbsoluteTransform();
			const glm::mat4 modelMatrix = absTransform.GetMatrix();

			//LOG_INFO("Translation for object with index %d : %f    %f    %f", i, modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);

			{
				MeshConstantBuffer constantBufferData;

				// All matrices sent to HLSL need to be converted to row-major(what D3D uses) from column-major(what glm uses)
				constantBufferData.LocalToClip = glm::transpose(inCurrentScene.GetMainCameraProj() * inCurrentScene.GetMainCameraLookAt() * modelMatrix);
				constantBufferData.LocalToWorldRotationOnly = glm::transpose(absTransform.GetRotationOnlyMatrix());

				MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(constantBufferData));
				memcpy(cBufferMap.CPUAddress, &constantBufferData, sizeof(constantBufferData));

				inCmdList->SetGraphicsRootConstantBufferView(2, cBufferMap.GPUAddress);
			}

			inCmdList->SetGraphicsRootShaderResourceView(1, D3D12Globals::GlobalMaterialsBuffer.GetCurrentGPUAddress());

			inCmdList->SetGraphicsRoot32BitConstant(3, modelChild->MatIndex, 0);

			inCmdList->SetGraphicsRootDescriptorTable(0, D3D12Globals::GlobalSRVHeap.GPUStart[D3D12Utility::CurrentFrameIndex]);

			inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			const D3D12_VERTEX_BUFFER_VIEW vbView = modelChild->VertexBuffer->VBView();
			const D3D12_INDEX_BUFFER_VIEW ibView = modelChild->IndexBuffer->IBView();
			inCmdList->IASetVertexBuffers(0, 1, &vbView);
			inCmdList->IASetIndexBuffer(&ibView);

			inCmdList->DrawIndexedInstanced(modelChild->IndexBuffer->IndexCount, 1, 0, 0, 0);

			++NrMeshesDrawn;
		}
	}
}


void DeferredBasePass::Execute(ID3D12GraphicsCommandList* inCmdList)
{
	PIXMarker Marker(inCmdList, "Draw GBuffer");

	D3D12Utility::TransitionResource(inCmdList, GBufferTextures.GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12Utility::TransitionResource(inCmdList, GBufferTextures.GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12Utility::TransitionResource(inCmdList, GBufferTextures.GBufferRoughness->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12Utility::TransitionResource(inCmdList, GBufferTextures.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);


	D3D12Globals::GlobalConstantsBuffer.ClearUsedMemory();

	// Populate Command List

	inCmdList->SetGraphicsRootSignature(m_GBufferMainMeshRootSignature);
	inCmdList->SetPipelineState(m_MainMeshPassPipelineState);

	// Handle RTs

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[3];
	renderTargets[0] = GBufferTextures.GBufferAlbedo->RTV;
	renderTargets[1] = GBufferTextures.GBufferNormal->RTV;
	renderTargets[2] = GBufferTextures.GBufferRoughness->RTV;
	inCmdList->OMSetRenderTargets(3, renderTargets, FALSE, &GBufferTextures.MainDepthBuffer->DSV);

	inCmdList->ClearRenderTargetView(GBufferTextures.GBufferAlbedo->RTV, D3D12Utility::ClearColor, 0, nullptr);
	inCmdList->ClearRenderTargetView(GBufferTextures.GBufferNormal->RTV, D3D12Utility::ClearColor, 0, nullptr);
	inCmdList->ClearRenderTargetView(GBufferTextures.GBufferRoughness->RTV, D3D12Utility::ClearColor, 0, nullptr);
	inCmdList->ClearDepthStencilView(GBufferTextures.MainDepthBuffer->DSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

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
		//DrawMeshNodesRecursively(inCmdList, children, currentScene, currModel->Materials);
	}

}
