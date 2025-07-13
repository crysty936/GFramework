#include "SkyboxPass.h"
#include "glm/common.hpp"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "Renderer/Drawable/Drawable.h"
#include "Renderer/RHI/D3D12/D3D12GraphicsTypes_Internal.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"
#include "Window/WindowsWindow.h"
#include "Core/AppCore.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "imgui.h"
#include "Math/MathUtils.h"
#include "Renderer/Drawable/ShapesUtils/BasicShapesData.h"
#include "DeferredBasePass.h"
#include "ArHosekSkyModel.h"
//#include "glm/ext/scalar_constants.hpp"

#include <d3d12.h>
#include <DirectXPackedVector.h>

// Constant Buffer
struct SkyboxConstantBuffer
{
	glm::mat4 Projection;
	glm::mat4 View;
	float SkyOnlyExposure;
	float padding[31];
};
static_assert((sizeof(SkyboxConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

SkyboxPass::SkyboxPass() = default;
SkyboxPass::~SkyboxPass() = default;

ID3D12RootSignature* m_SkyboxRootSignature;
ID3D12PipelineState* m_SkyboxPipelineState;

eastl::shared_ptr<D3D12IndexBuffer> SkyboxIndexBuffer = nullptr;
eastl::shared_ptr<D3D12VertexBuffer> SkyboxVertexBuffer = nullptr;


glm::vec3 XYZToDirection(const uint64_t inX, const uint64_t inY, const uint64_t inZ, const uint64_t inWidth, const uint64_t inHeight)
{
	// Move coordinate to pixel center, remap to 0..1, remap to -1..1
	const float u = ((inX + 0.5f) / float(inWidth)) * 2.0f - 1.0f;
	float v = ((inY + 0.5f) / float(inHeight)) * 2.0f - 1.0f;
	// D3D12 v goes top to bottom
	v *= -1.0f;

	glm::vec3 dir = glm::vec3(0.0f, 0.0f, 0.0f);

	// https://en.wikipedia.org/wiki/Cube_mapping#/media/File:Cube_map.svg
	// v is already reversed so it goes exactly like in the illustration
	switch (inZ)
	{
	case 0:
		dir = glm::normalize(glm::vec3(1.0f, v, -u));
		break;
	case 1:
		dir = glm::normalize(glm::vec3(-1.0f, v, u));
		break;
	case 2:
		dir = glm::normalize(glm::vec3(u, 1.0f, -v));
		break;
	case 3:
		dir = glm::normalize(glm::vec3(u, -1.0f, v));
		break;
	case 4:
		dir = glm::normalize(glm::vec3(u, v, 1.0f));
		break;
	case 5:
		dir = glm::normalize(glm::vec3(-u, v, -1.0f));
		break;
	}


	return dir;


}

float AngleBetween(const glm::vec3& inDir1, const glm::vec3& inDir2)
{
	return glm::acos(glm::max<float>(0.00001f, glm::dot(inDir1, inDir2)));
}

DirectX::PackedVector::XMHALF4 ToHalf4(const glm::vec4 inFloat)
{
	DirectX::PackedVector::XMHALF4 res(inFloat.x, inFloat.y, inFloat.z, inFloat.w);
	//DirectX::PackedVector::XMStoreHalf4(&res, DirectX::XMVectorSet(x, y, z, w));

	return res;
}

//struct half4
//{
//	uint16_t x;
//	uint16_t y;
//	uint16_t z;
//	uint16_t w;
//
//	half4(float x, float y, float z, float w)
//	{
//		DirectX::PackedVector::XMStoreHalf4(reinterpret_cast<DirectX::PackedVector::XMHALF4*>(this), DirectX::XMVectorSet(x, y, z, w));
//	}
//};

void SkyboxPass::InitSkyModel(ID3D12GraphicsCommandList* inCmdList)
{
	if (bInitialized && SunDirection == SunDirectionCache && GroundAlbedo == GroundAlbedoCache && Turbidity == TurbidityCache)
	{
		return;
	}

	bInitialized = true;
	SunDirectionCache = SunDirection;
	GroundAlbedoCache = GroundAlbedo;
	TurbidityCache = Turbidity;

	const glm::vec3 sunDirNormalized = glm::normalize(SunDirectionCache);

	// Theta is angle between top and vector
	const float thetaSpherical = glm::acos(glm::dot(sunDirNormalized, glm::vec3(0.f, 1.f, 0.f))); // Basically acos(y).

	// Elevation is angle between bottom plane(xz) or horizon and vector. 90 degrees - theta
	const float elevation = (glm::pi<float>() / 2.f) - thetaSpherical; // pi/2 radians - theta radians

	if (StateR)
	{
		arhosekskymodelstate_free(StateR);
		arhosekskymodelstate_free(StateG);
		arhosekskymodelstate_free(StateB);

		StateR = nullptr;
		StateG = nullptr;
		StateB = nullptr;
	}

	StateR = arhosek_rgb_skymodelstate_alloc_init(Turbidity, GroundAlbedo.x, elevation);
	StateG = arhosek_rgb_skymodelstate_alloc_init(Turbidity, GroundAlbedo.y, elevation);
	StateB = arhosek_rgb_skymodelstate_alloc_init(Turbidity, GroundAlbedo.z, elevation);

	const uint64_t cubemapRes = 128;
	const uint64_t numTexels = cubemapRes * cubemapRes * 6;

	eastl::vector<glm::vec4> texels(numTexels);

	for (uint64_t z = 0; z < 6; ++z)
	{
		for (uint64_t y = 0; y < cubemapRes; ++y)
		{
			for (uint64_t x = 0; x < cubemapRes; ++x)
			{
				const glm::vec3 dir = XYZToDirection(x, y, z, cubemapRes, cubemapRes);
				glm::vec3 radiance;

				//https://cgg.mff.cuni.cz/projects/SkylightModelling/HosekWilkie_SkylightModel_SIGGRAPH2012_Preprint.pdf
				// Section 5.1
				// Theta is angle between sample dir and zenith
				// Gamma is angle between sample dir and solar point, or solar dir

				const float sampleTheta = AngleBetween(dir, glm::vec3(0.f, 1.f, 0.f));
				const float sampleGamma = AngleBetween(dir, sunDirNormalized);

				radiance.x = float(arhosek_tristim_skymodel_radiance(StateR, sampleTheta, sampleGamma, 0));
				radiance.y = float(arhosek_tristim_skymodel_radiance(StateG, sampleTheta, sampleGamma, 1));
				radiance.z = float(arhosek_tristim_skymodel_radiance(StateB, sampleTheta, sampleGamma, 2));

				// Convert radiometric units to photometric using standard luminous efficacy of 683 lm/W
				radiance *= 683.f;

				const uint64_t texelIdx = (z * cubemapRes * cubemapRes) + (y * cubemapRes) + x;
				texels[texelIdx] =  glm::vec4(radiance, 1.f);

			}
		}
	}

	Cubemap = D3D12RHI::Get()->CreateTexture2D(cubemapRes, cubemapRes, DXGI_FORMAT_R32G32B32A32_FLOAT, inCmdList, L"Skybox Cubemap", texels.data(), true);
}


void SkyboxPass::Init()
{
	// Create screen quad data
	SkyboxIndexBuffer = D3D12RHI::Get()->CreateIndexBuffer(BasicShapesData::GetSkyboxIndices(), BasicShapesData::GetSkyboxIndicesCount());

	// Create the vertex buffer.
	{
		VertexInputLayout vbLayout;
		vbLayout.Push<float>(3, VertexInputType::Position);

		SkyboxVertexBuffer = D3D12RHI::Get()->CreateVertexBuffer(vbLayout, BasicShapesData::GetSkyboxVertices(), BasicShapesData::GetSkyboxVerticesCount(), SkyboxIndexBuffer);
	}


	// Skybox Pass signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[3];

		// Main CBV_SRV_UAV heap
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		//D3D12_DESCRIPTOR_RANGE1 rangesPS[1];
		//rangesPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		//rangesPS[0].NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		//rangesPS[0].BaseShaderRegister = 0;
		//rangesPS[0].RegisterSpace = 0;
		//rangesPS[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		//rangesPS[0].OffsetInDescriptorsFromTableStart = 0;

		rootParameters[0].DescriptorTable.pDescriptorRanges = D3D12Utility::GetGlobalHeapDescriptorRangeDescs();
		rootParameters[0].DescriptorTable.NumDescriptorRanges = D3D12Utility::GetGlobalHeapDescriptorRangeDescsCount();

		// Constant Buffer
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].Constants.RegisterSpace = 1;
		rootParameters[2].Constants.ShaderRegister = 0;
		rootParameters[2].Constants.Num32BitValues = 1;

		//////////////////////////////////////////////////////////////////////////

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		//sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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

		m_SkyboxRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
		m_SkyboxRootSignature->SetName(L"SkyBox Root Signature");
	}

	// Basic Shapes PSO
	{
		eastl::string fullPath = "../Data/Shaders/D3D12/"; ;
		fullPath += "SkyboxMesh.hlsl";

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
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::WriteDisabled);
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

void SkyboxPass::Execute(ID3D12GraphicsCommandList* inCmdList, D3D12RenderTarget2D& inRT, SceneTextures& inGBuffer)
{
	PIXMarker Marker(inCmdList, "Skybox");

	ImGui::Begin("Skybox");

	ImGui::SliderFloat("Turbidity", &Turbidity, 0.f, 32.f);
	ImGui::DragFloat3("Sun Dir", &SunDirection.x, 0.05f, -360.f, 360.f);
	ImGui::DragFloat3("GroundAlbedo", &GroundAlbedo.x, 0.05f, 0.f, 1.f);
	ImGui::SliderFloat("Sky Exposure", &SkyExposure, -32.f, 32.f);

	ImGui::End();

	InitSkyModel(inCmdList);

	D3D12Utility::TransitionResource(inCmdList, inGBuffer.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_READ);

	// Populate Command List

	inCmdList->SetGraphicsRootSignature(m_SkyboxRootSignature);
	inCmdList->SetPipelineState(m_SkyboxPipelineState);

	// Handle RTs

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[1];
	renderTargets[0] = inRT.RTV;
	inCmdList->OMSetRenderTargets(1, renderTargets, false, &inGBuffer.MainDepthBuffer->DSV);

	{
		SkyboxConstantBuffer constantBufferData;

		SceneManager& sManager = SceneManager::Get();
		const Scene& currentScene = sManager.GetCurrentScene();

		// All matrices sent to HLSL need to be converted to row-major(what D3D uses) from column-major(what glm uses)
		constantBufferData.Projection = glm::transpose(currentScene.GetMainCameraProj());
		constantBufferData.View = glm::transpose(currentScene.GetMainCameraLookAt());
		constantBufferData.SkyOnlyExposure = SkyExposure;

		MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(constantBufferData));
		memcpy(cBufferMap.CPUAddress, &constantBufferData, sizeof(constantBufferData));

		inCmdList->SetGraphicsRootDescriptorTable(0, D3D12Globals::GlobalSRVHeap.GPUStart[D3D12Utility::CurrentFrameIndex]);
		inCmdList->SetGraphicsRootConstantBufferView(1, cBufferMap.GPUAddress);
		inCmdList->SetGraphicsRoot32BitConstant(2, Cubemap->SRVIndex, 0);

		inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		const D3D12_VERTEX_BUFFER_VIEW vbView = SkyboxVertexBuffer->VBView();
		const D3D12_INDEX_BUFFER_VIEW ibView = SkyboxIndexBuffer->IBView();

		inCmdList->IASetVertexBuffers(0, 1, &vbView);
		inCmdList->IASetIndexBuffer(&ibView);

		inCmdList->DrawIndexedInstanced(BasicShapesData::GetSkyboxIndicesCount(), 1, 0, 0, 0);
	}

	D3D12Utility::TransitionResource(inCmdList, inGBuffer.MainDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


}
