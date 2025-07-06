#include "DebugTexturesPass.h"
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

// 256 byte aligned
struct DebugTexturesConstantBuffer
{
	glm::vec4 TranslationScale;
	//glm::mat4 TranslationScale;
	//float4x4 ProjInv;
	//float4x4 Proj;

	//float4 ViewPos;
	//float4 LightDir;
	float Padding[51];
};


static ID3D12RootSignature* m_DebugTexturesRS;
static ID3D12PipelineState* m_DebugTexturesPSO;

static eastl::shared_ptr<D3D12IndexBuffer> ScreenQuadIndexBuffer = nullptr;
static eastl::shared_ptr<D3D12VertexBuffer> ScreenQuadVertexBuffer = nullptr;

void DebugTexturePass::Init()
{
	{
		D3D12_ROOT_PARAMETER1 rootParameters[2] = {};

		// Constant Buffer
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
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

		// Texture to present
		texturesRange[0].NumDescriptors = 1;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(texturesRange);
		rootParameters[1].DescriptorTable.pDescriptorRanges = &texturesRange[0];

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

		m_DebugTexturesRS = D3D12RHI::Get()->CreateRootSignature(rootSignatureDesc);
	}


	// Lighting Quad PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "DebugTexturesPass.hlsl";

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
		psoDesc.pRootSignature = m_DebugTexturesRS;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;
		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::Disabled);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::Disabled);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_DebugTexturesPSO)));
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

void DrawTexturesArray(const eastl::vector<eastl::string*>& inTextureNames, const char* inName, const int inStartIdx, const int inEndIdx, int& inOutCurentItemIdx)
{
	// Handle listbox being clipped
	if (!ImGui::BeginListBox(inName))
	{
		return;
	}

	const int32_t numTextures = inTextureNames.size();
	ASSERT(inStartIdx >= 0 && numTextures > inEndIdx);

	for (int32_t elIdx = inStartIdx; elIdx <= inEndIdx; ++elIdx)
	{
		const eastl::string& textureName = *inTextureNames[elIdx];
		ASSERT(!textureName.empty());

		bool is_selected = (inOutCurentItemIdx == elIdx);
		if (ImGui::Selectable(textureName.c_str(), is_selected))
		{
			if (inOutCurentItemIdx == elIdx)
			{
				inOutCurentItemIdx = -1;
				is_selected = false;
			}
			else
			{
				inOutCurentItemIdx = elIdx;
			}
		}

		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
		if (is_selected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}

	ImGui::EndListBox();
}

using TextureType = DebugTexturePass::TextureType;

struct TexturesData 
{
	int32_t SelectedItemIdx = -1;
	TextureType SelectedTextureType = TextureType::Max;
};

TexturesData DrawAllTextures()
{
	eastl::vector<eastl::string*> allTextures;

	int32_t texturesOffsets[static_cast<int32_t>(TextureType::Max)];

	// Separate lists for plain textures, render targets and depth buffers
	{
		texturesOffsets[static_cast<int32_t>(TextureType::Standard)] = 0;
		const eastl::vector<eastl::shared_ptr<D3D12Texture2D>>& textures = D3D12RHI::Get()->GetLiveTextures();

		for (const eastl::shared_ptr<D3D12Texture2D>& tex : textures)
		{
			allTextures.push_back(&tex->Name);
		}
	}

	{
		texturesOffsets[static_cast<int32_t>(TextureType::Render)] = allTextures.size();
		const eastl::vector<eastl::shared_ptr<D3D12RenderTarget2D>>& RTs = D3D12RHI::Get()->GetRTs();

		for (const eastl::shared_ptr<D3D12RenderTarget2D>& RT : RTs)
		{
			allTextures.push_back(&RT->Texture->Name);
		}
	}

	{
		texturesOffsets[static_cast<int32_t>(TextureType::Depth)] = allTextures.size();
		const eastl::vector<eastl::shared_ptr<D3D12DepthBuffer>>& DTs = D3D12RHI::Get()->GetDTs();

		for (const eastl::shared_ptr<D3D12DepthBuffer>& DT : DTs)
		{
			allTextures.push_back(&DT->Texture->Name);
		}
	}

	static int selectedItemIdx = -1;
	DrawTexturesArray(allTextures, "Standard", texturesOffsets[(int32_t)TextureType::Standard], texturesOffsets[(int32_t)TextureType::Render] - 1, selectedItemIdx);
	DrawTexturesArray(allTextures, "RenderTargets", texturesOffsets[(int32_t)TextureType::Render], texturesOffsets[(int32_t)TextureType::Depth] - 1, selectedItemIdx);
	DrawTexturesArray(allTextures, "Depth", texturesOffsets[(int32_t)TextureType::Depth], allTextures.size() - 1, selectedItemIdx);

	TextureType resType = TextureType::Max;
	if (selectedItemIdx != -1)
	{
		// Figure out what type of texture is selected

		for (int32_t i = (int32_t)TextureType::Standard; i < (int32_t)TextureType::Max; ++i)
		{
			const int32_t currOffset = texturesOffsets[i];
			if (selectedItemIdx > currOffset)
			{
				resType = static_cast<TextureType>(i);
			}
			else
			{
				break;
			}
		}
	}

	return { selectedItemIdx, resType };
}

void DebugTexturePass::Execute(ID3D12GraphicsCommandList* inCmdList, const D3D12RenderTarget2D& inTarget)
{
	ImGui::Begin("Textures");

	ImGui::DragFloat2("Offset", &Offset.x, 0.01f, -1.f, 1.f);
	ImGui::DragFloat2("Scale", &Scale.x, 0.01f, 0.f, 1.f);


	const TexturesData selectedTextureData = DrawAllTextures();

	ImGui::End();

	if (selectedTextureData.SelectedItemIdx == -1)
	{
		return;
	}

	

	DrawTexture(inCmdList, inTarget, selectedTextureData.SelectedItemIdx, selectedTextureData.SelectedTextureType);
}

void DebugTexturePass::DrawTexture(ID3D12GraphicsCommandList* inCmdList, const D3D12RenderTarget2D& inTarget, const int32_t selectedTexIndex, const TextureType inType)
{
	PIXMarker Marker(inCmdList, "Render Debug Textures");

	// Draw screen quad

	inCmdList->SetGraphicsRootSignature(m_DebugTexturesRS);
	inCmdList->SetPipelineState(m_DebugTexturesPSO);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
	renderTargets[0] = inTarget.RTV;

	inCmdList->OMSetRenderTargets(1, renderTargets, FALSE, nullptr);

	{
		DebugTexturesConstantBuffer constantsBuffer = {};
		constantsBuffer.TranslationScale = glm::vec4(Offset.x, Offset.y, Scale.x, Scale.y);

		// Use temp buffer in main constant buffer
		MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(DebugTexturesConstantBuffer));
		memcpy(cBufferMap.CPUAddress, &constantsBuffer, sizeof(DebugTexturesConstantBuffer));
		inCmdList->SetGraphicsRootConstantBufferView(0, cBufferMap.GPUAddress);
	}

	//inCmdList->SetGraphicsRootDescriptorTable(1, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.GBufferAlbedo->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));
	//inCmdList->SetGraphicsRootDescriptorTable(2, D3D12Globals::GlobalSRVHeap.GetGPUHandle(inSceneTextures.MainDepthBuffer->Texture->SRVIndex, D3D12Utility::CurrentFrameIndex));

	inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const D3D12_VERTEX_BUFFER_VIEW vbView = ScreenQuadVertexBuffer->VBView();
	const D3D12_INDEX_BUFFER_VIEW ibView = ScreenQuadIndexBuffer->IBView();
	inCmdList->IASetVertexBuffers(0, 1, &vbView);
	inCmdList->IASetIndexBuffer(&ibView);

	inCmdList->DrawIndexedInstanced(ScreenQuadIndexBuffer->IndexCount, 1, 0, 0, 0);
}

