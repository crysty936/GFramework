#include "Skybox.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
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
#include "../Drawable/ShapesUtils/BasicShapesData.h"

// Constant Buffer
struct SkyboxConstantBuffer
{
	glm::mat4 Projection;
	glm::mat4 View;
	float padding[32];
};
static_assert((sizeof(SkyboxConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");


SkyboxPass::SkyboxPass() = default;
SkyboxPass::~SkyboxPass() = default;

ID3D12RootSignature* m_SkyboxRootSignature;
ID3D12PipelineState* m_SkyboxPipelineState;

eastl::shared_ptr<D3D12IndexBuffer> SkyboxIndexBuffer = nullptr;
eastl::shared_ptr<D3D12VertexBuffer> SkyboxVertexBuffer = nullptr;

void SkyboxPass::Init()
{
	// Skybox Pass signature
	{
		//D3D12_ROOT_PARAMETER1 rootParameters[0];

		// Main CBV_SRV_UAV heap
		//rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		//D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
		//rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		//rangesPS[0].NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		//rangesPS[0].BaseShaderRegister = 0;
		//rangesPS[0].RegisterSpace = 0;
		//rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		//rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

		//rootParameters[0].DescriptorTable.pDescriptorRanges = &rangesPS[0];
		//rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(rangesPS);

		//rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		//rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		//rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		//rootParameters[1].Descriptor.RegisterSpace = 0;
		//rootParameters[1].Descriptor.ShaderRegister = 0;

		//rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		//rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		//rootParameters[2].Constants.RegisterSpace = 0;
		//rootParameters[2].Constants.ShaderRegister = 0;
		//rootParameters[2].Constants.Num32BitValues = 1;



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
		//rootSignatureDesc.NumParameters = _countof(rootParameters);
		//rootSignatureDesc.pParameters = &rootParameters[0];
		rootSignatureDesc.NumParameters = 0;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags = rootSignatureFlags;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
		versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

		m_SkyboxRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
		m_SkyboxRootSignature->SetName(L"SkyBox Root Signature");
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
		psoDesc.pRootSignature = m_SkyboxRootSignature;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;
		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::BackFaceCull);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::AlwaysFail_WriteDisabled);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_SkyboxPipelineState)));
		m_SkyboxPipelineState->SetName(L"Skybox Pipeline State");

	}

}

void SkyboxPass::Execute(ID3D12GraphicsCommandList* inCmdList)
{
	PIXMarker Marker(inCmdList, "Skybox");

	// Populate Command List

	inCmdList->SetGraphicsRootSignature(m_SkyboxRootSignature);
	inCmdList->SetPipelineState(m_SkyboxPipelineState);

	// Handle RTs

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
	//renderTargets[0] = GBufferTextures.GBufferAlbedo->RTV;
	//inCmdList->OMSetRenderTargets(3, renderTargets, FALSE, &GBufferTextures.MainDepthBuffer->DSV);

		{
			SkyboxConstantBuffer constantBufferData;

			SceneManager& sManager = SceneManager::Get();
			const Scene& currentScene = sManager.GetCurrentScene();

			// All matrices sent to HLSL need to be converted to row-major(what D3D uses) from column-major(what glm uses)
			constantBufferData.Projection = glm::transpose(currentScene.GetMainCameraProj());
			constantBufferData.View = glm::transpose(currentScene.GetMainCameraLookAt());

			MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(constantBufferData));
			memcpy(cBufferMap.CPUAddress, &constantBufferData, sizeof(constantBufferData));

			inCmdList->SetGraphicsRootConstantBufferView(2, cBufferMap.GPUAddress);
			inCmdList->SetGraphicsRootShaderResourceView(1, D3D12Globals::GlobalMaterialsBuffer.GetCurrentGPUAddress());

			inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			const D3D12_VERTEX_BUFFER_VIEW vbView = SkyboxVertexBuffer->VBView();
			const D3D12_INDEX_BUFFER_VIEW ibView = SkyboxIndexBuffer->IBView();

			inCmdList->IASetVertexBuffers(0, 1, &vbView);
			inCmdList->IASetIndexBuffer(&ibView);

			inCmdList->DrawIndexedInstanced(BasicShapesData::GetSkyboxIndicesCount(), 1, 0, 0, 0);
		}

}
