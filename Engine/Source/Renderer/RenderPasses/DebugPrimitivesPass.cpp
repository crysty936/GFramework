#include "DebugPrimitivesPass.h"
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
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Renderer/DrawDebugHelpers.h"

// Constant Buffer
struct DebugConstants
{
	glm::mat4 WorldToClip;
	glm::mat4 CameraToWorld;
	uint32_t Padding[32];
};
static_assert((sizeof(DebugConstants) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

struct PackedDebugPointInstanceData
{
	glm::vec3 Translation;
	float Scale;
	uint32_t Color;
	uint32_t Padding[3];
};
static_assert((sizeof(PackedDebugPointInstanceData) % 16) == 0, "Structs in Structured Buffers have to be 16-byte aligned");

struct PackedDebugLineInstanceData
{
	glm::vec3 Position[2];
	uint32_t Color;
	uint32_t Padding;
};
static_assert((sizeof(PackedDebugLineInstanceData) % 16) == 0, "Structs in Structured Buffers have to be 16-byte aligned");

ID3D12RootSignature* m_DebugPrimitivesQuadPointsRootSignature;
ID3D12PipelineState* m_DebugPrimitivesQuadPointsPipelineState;
ID3D12PipelineState* m_DebugPrimitivesLinesPipelineState;
D3D12StructuredBuffer m_PointsBuffer;
D3D12StructuredBuffer m_LinesBuffer;
//D3D12RawBuffer m_PackedPointsBuffer;

#define MAX_NR_POINTS 1024
#define MAX_NR_LINES 1024

void DebugPrimitivesPass::Init()
{
	m_PointsBuffer.Init(MAX_NR_POINTS, sizeof(PackedDebugPointInstanceData));
	m_LinesBuffer.Init(MAX_NR_LINES, sizeof(PackedDebugLineInstanceData));

	// Debug Point Quads Root Signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[2];

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = &rootParameters[0];
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.Flags = rootSignatureFlags;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
		versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

		m_DebugPrimitivesQuadPointsRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
		m_DebugPrimitivesQuadPointsRootSignature->SetName(L"Debug Quad Points Root Signature");
	}

	// Debug Point Quads PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "DebugDraw.hlsl";

		CompiledShaderResult meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath, "VSMainPoints", "PSMainPoints");

		// shader bytecodes
		D3D12_SHADER_BYTECODE vsByteCode;
		vsByteCode.pShaderBytecode = meshShaderPair.VSByteCode->GetBufferPointer();
		vsByteCode.BytecodeLength = meshShaderPair.VSByteCode->GetBufferSize();

		D3D12_SHADER_BYTECODE psByteCode;
		psByteCode.pShaderBytecode = meshShaderPair.PSByteCode->GetBufferPointer();
		psByteCode.BytecodeLength = meshShaderPair.PSByteCode->GetBufferSize();

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_DebugPrimitivesQuadPointsRootSignature;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;
		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::Disabled);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::WriteDisabled);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_DebugPrimitivesQuadPointsPipelineState)));
		m_DebugPrimitivesQuadPointsPipelineState->SetName(L"Debug Point Quads Pipeline State");
	}

	// Debug Lines PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "DebugDraw.hlsl";

		CompiledShaderResult meshShaderPair = D3D12RHI::Get()->CompileGraphicsShaderFromFile(fullPath, "VSMainLines", "PSMainLines");

		// shader bytecodes
		D3D12_SHADER_BYTECODE vsByteCode;
		vsByteCode.pShaderBytecode = meshShaderPair.VSByteCode->GetBufferPointer();
		vsByteCode.BytecodeLength = meshShaderPair.VSByteCode->GetBufferSize();

		D3D12_SHADER_BYTECODE psByteCode;
		psByteCode.pShaderBytecode = meshShaderPair.PSByteCode->GetBufferPointer();
		psByteCode.BytecodeLength = meshShaderPair.PSByteCode->GetBufferSize();

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_DebugPrimitivesQuadPointsRootSignature;
		psoDesc.VS = vsByteCode;
		psoDesc.PS = psByteCode;
		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::Disabled);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::WriteDisabled);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_DebugPrimitivesLinesPipelineState)));
		m_DebugPrimitivesLinesPipelineState->SetName(L"Debug Lines Pipeline State");
	}
}

void DebugPrimitivesPass::Execute(struct ID3D12GraphicsCommandList* inCmdList, const class D3D12DepthBuffer& inDepthBuffer, const D3D12RenderTarget2D& inTarget)
{
	PIXMarker Marker(inCmdList, "Render Debug Primitives");

	// Generate buffer data
	DrawDebugManager& manager = DrawDebugManager::Get();
	const eastl::vector<DebugPoint>& debugPoints = manager.GetDebugPoints();
	ASSERT(debugPoints.size() < MAX_NR_POINTS);

	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	glm::mat4 projection = currentScene.GetMainCameraProj();

	// Convert projection to infinite
	projection[2][2] = 1.0f;
	projection[3][2] = -2.0f * currentScene.GetCurrentCamera()->GetNear();
	// TODO: This messes up the depth occlusion because result is different than standard perspective.

	DebugConstants constData;
	constData.WorldToClip = glm::transpose(projection * currentScene.GetMainCameraLookAt());
	constData.CameraToWorld = glm::transpose(glm::inverse(currentScene.GetMainCameraLookAt()));

	MapResult constBuffer{};

	if (debugPoints.size() > 0)
	{
		D3D12Utility::TransitionResource(inCmdList, inDepthBuffer.Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_READ);

		eastl::vector<PackedDebugPointInstanceData> pointsInstanceData;
		for (uint32_t i = 0; i < debugPoints.size(); ++i)
		{
			PackedDebugPointInstanceData newPoint = {};

			const DebugPoint& pointDataSource = debugPoints[i];

			newPoint.Translation = pointDataSource.Location;
			newPoint.Scale = 0.1f * pointDataSource.Size;
			newPoint.Color = RenderUtils::ConvertToRGBA8(glm::vec4(pointDataSource.Color, 1.f));

			pointsInstanceData.push_back(newPoint);
		}

		m_PointsBuffer.UploadDataAllFrames(&pointsInstanceData[0], sizeof(PackedDebugPointInstanceData) * pointsInstanceData.size());

		// Populate Command List

		inCmdList->SetGraphicsRootSignature(m_DebugPrimitivesQuadPointsRootSignature);
		inCmdList->SetPipelineState(m_DebugPrimitivesQuadPointsPipelineState);

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
		renderTargets[0] = inTarget.RTV;

		inCmdList->OMSetRenderTargets(1, renderTargets, false, &inDepthBuffer.DSV);

		// Use temp buffer in main constant buffer
		constBuffer = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(constData));
		memcpy(constBuffer.CPUAddress, &constData, sizeof(constData));

		inCmdList->SetGraphicsRootConstantBufferView(0, constBuffer.GPUAddress);
		inCmdList->SetGraphicsRootShaderResourceView(1, m_PointsBuffer.GetCurrentGPUAddress());

		inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		inCmdList->DrawIndexedInstanced(6, pointsInstanceData.size(), 0, 0, 0);
	}

	const eastl::vector<DebugLine>& debugLines = manager.GetDebugLines();
	ASSERT(debugLines.size() < MAX_NR_LINES);

	if (debugLines.size() > 0)
	{
		eastl::vector<PackedDebugLineInstanceData> linesInstanceData;
		for (uint32_t i = 0; i < debugLines.size(); ++i)
		{
			PackedDebugLineInstanceData newLine = {};

			const DebugLine& lineDataSource = debugLines[i];

			newLine.Position[0] = lineDataSource.Start;
			newLine.Position[1] = lineDataSource.End;
			newLine.Color = RenderUtils::ConvertToRGBA8(glm::vec4(lineDataSource.Color, 1.f));

			linesInstanceData.push_back(newLine);
		}

		m_LinesBuffer.UploadDataAllFrames(&linesInstanceData[0], sizeof(PackedDebugLineInstanceData) * linesInstanceData.size());


		// Populate Command List

		inCmdList->SetGraphicsRootSignature(m_DebugPrimitivesQuadPointsRootSignature);
		inCmdList->SetPipelineState(m_DebugPrimitivesLinesPipelineState);

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
		renderTargets[0] = inTarget.RTV;

		inCmdList->OMSetRenderTargets(1, renderTargets, false, &inDepthBuffer.DSV);

		// Use temp buffer in main constant buffer
		if (!constBuffer.CPUAddress)
		{
			constBuffer = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(constData));
		}
		memcpy(constBuffer.CPUAddress, &constData, sizeof(constData));

		inCmdList->SetGraphicsRootConstantBufferView(0, constBuffer.GPUAddress);
		inCmdList->SetGraphicsRootShaderResourceView(1, m_LinesBuffer.GetCurrentGPUAddress());

		inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		inCmdList->DrawIndexedInstanced(2, linesInstanceData.size(), 0, 0, 0);


		D3D12Utility::TransitionResource(inCmdList, inDepthBuffer.Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	manager.ClearDebugData();
}
