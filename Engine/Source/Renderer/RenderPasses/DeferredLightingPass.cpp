#include "DeferredLightingPass.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "Renderer/RHI/Resources/RHITexture.h"
#include <d3d12.h>
#include "Window/WindowsWindow.h"
#include "Window/WindowProperties.h"
#include "Renderer/RHI/D3D12/D3D12Utility.h"
#include "DeferredBasePass.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "imgui.h"
#include "Renderer/Drawable/ShapesUtils/BasicShapesData.h"
#include "Core/AppCore.h"

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

ID3D12RootSignature* m_LightingRootSignature;
ID3D12PipelineState* m_LightingPipelineState;

// TODO: Convert to compute
eastl::shared_ptr<D3D12IndexBuffer> ScreenQuadIndexBuffer = nullptr;
eastl::shared_ptr<D3D12VertexBuffer> ScreenQuadVertexBuffer = nullptr;

void DeferredLightingPass::Init()
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



	// Create screen quad data
	ScreenQuadIndexBuffer = D3D12RHI::Get()->CreateIndexBuffer(BasicShapesData::GetQuadIndices(), BasicShapesData::GetQuadIndicesCount());

	// Create the vertex buffer.
	{
		VertexInputLayout vbLayout;
		vbLayout.Push<float>(3, VertexInputType::Position);
		vbLayout.Push<float>(2, VertexInputType::TexCoords);

		ScreenQuadVertexBuffer = D3D12RHI::Get()->CreateVertexBuffer(vbLayout, BasicShapesData::GetQuadVertices(), BasicShapesData::GetQuadVerticesCount(), ScreenQuadIndexBuffer);
	}


}

void DeferredLightingPass::Execute(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures, const D3D12RenderTarget2D& inTarget)
{
	D3D12Utility::TransitionResource(inCmdList, inSceneTextures.GBufferAlbedo->Texture->Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	D3D12Utility::TransitionResource(inCmdList, inSceneTextures.GBufferNormal->Texture->Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	D3D12Utility::TransitionResource(inCmdList, inSceneTextures.GBufferRoughness->Texture->Resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	D3D12Utility::TransitionResource(inCmdList, inSceneTextures.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	RenderLighting(inCmdList, inSceneTextures, inTarget);
}


void DeferredLightingPass::RenderLighting(ID3D12GraphicsCommandList* inCmdList, SceneTextures& inSceneTextures, const D3D12RenderTarget2D& inTarget)
{
	PIXMarker Marker(inCmdList, "Render Deferred Lighting");

	// Draw screen quad

	inCmdList->ClearRenderTargetView(inTarget.RTV, D3D12Utility::ClearColor, 0, nullptr);

	inCmdList->SetGraphicsRootSignature(m_LightingRootSignature);
	inCmdList->SetPipelineState(m_LightingPipelineState);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
	renderTargets[0] = inTarget.RTV;

	inCmdList->OMSetRenderTargets(1, renderTargets, FALSE, nullptr);

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
		inCmdList->SetGraphicsRootConstantBufferView(0, cBufferMap.GPUAddress);
	}

	inCmdList->SetGraphicsRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.GBufferAlbedo->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));
	inCmdList->SetGraphicsRootDescriptorTable(2, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

	inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const D3D12_VERTEX_BUFFER_VIEW vbView = ScreenQuadVertexBuffer->VBView();
	const D3D12_INDEX_BUFFER_VIEW ibView = ScreenQuadIndexBuffer->IBView();
	inCmdList->IASetVertexBuffers(0, 1, &vbView);
	inCmdList->IASetIndexBuffer(&ibView);

	inCmdList->DrawIndexedInstanced(ScreenQuadIndexBuffer->IndexCount, 1, 0, 0, 0);
}

