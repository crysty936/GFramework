#include "ShadowPass.h"
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
	uint32_t Padding[48];
};
static_assert((sizeof(MeshConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

ID3D12RootSignature* m_ShadowPassRootSignature;
ID3D12PipelineState* m_ShadowPassPSO;

void ShadowPass::Init()
{
	// Textures
	ShadowDepthBuffer = D3D12RHI::Get()->CreateDepthBuffer(1024, 1024, L"Shadow Depth Buffer", ETextureState::Shader_Resource);

	// Root Signature
	{
		D3D12_ROOT_PARAMETER1 rootParameters[1];

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		//////////////////////////////////////////////////////////////////////////

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = &rootParameters[0];
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.Flags = rootSignatureFlags;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
		versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

		m_ShadowPassRootSignature = D3D12RHI::Get()->CreateRootSignature(versionedRootSignatureDesc);
	}

	// PSO
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

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_ShadowPassRootSignature;
		psoDesc.VS = vsByteCode;

		psoDesc.RasterizerState = D3D12Utility::GetRasterizerState(ERasterizerState::BackFaceCull);
		psoDesc.BlendState = D3D12Utility::GetBlendState(EBlendState::Disabled);
		psoDesc.DepthStencilState = D3D12Utility::GetDepthState(EDepthState::WriteEnabled);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.SampleDesc.Count = 1;

		DXAssert(D3D12Globals::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_ShadowPassPSO)));
	}



}

uint64_t TestNrMeshesToDraw = uint64_t(-1);
uint64_t NrMeshesDrawn = 0;

void DrawMeshNodesRecursively(ID3D12GraphicsCommandList* inCmdList, const eastl::vector<TransformObjPtr>& inChildNodes, const Scene& inCurrentScene, const eastl::vector<MeshMaterial>& inMaterials, const glm::mat4& inShadowMatrix)
{
	for (const TransformObjPtr& child : inChildNodes)
	{
		const TransformObject* childPtr = child.get();

		DrawMeshNodesRecursively(inCmdList, childPtr->GetChildren(), inCurrentScene, inMaterials, inShadowMatrix);

		//if (NrMeshesDrawn >= TestNrMeshesToDraw)
		//{
		//	return;
		//}

		if (const MeshNode* modelChild = dynamic_cast<const MeshNode*>(childPtr))
		{
			if (modelChild->MatIndex == uint32_t(-1) || inMaterials.size() == 0)
			{
				continue;
			}

			const Transform& absTransform = modelChild->GetAbsoluteTransform();
			const glm::mat4 modelMatrix = absTransform.GetMatrix();

			{
				MeshConstantBuffer constantBufferData;


				const glm::mat4 Proj = glm::orthoLH_ZO(20, -20, 20, -20, 0, 100);
				//constantBufferData.LocalToClip = glm::transpose(Proj * inCurrentScene.GetMainCameraLookAt() * modelMatrix);
				constantBufferData.LocalToClip = glm::transpose(Proj * inShadowMatrix * modelMatrix);
				//constantBufferData.LocalToClip = glm::transpose(inCurrentScene.GetMainCameraProj() * inCurrentScene.GetMainCameraLookAt() * modelMatrix);

				MapResult cBufferMap = D3D12Globals::GlobalConstantsBuffer.ReserveTempBufferMemory(sizeof(constantBufferData));
				memcpy(cBufferMap.CPUAddress, &constantBufferData, sizeof(constantBufferData));

				inCmdList->SetGraphicsRootConstantBufferView(0, cBufferMap.GPUAddress);
			}

			inCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			const D3D12_VERTEX_BUFFER_VIEW vbView = modelChild->VertexBuffer->VBView();
			const D3D12_INDEX_BUFFER_VIEW ibView = modelChild->IndexBuffer->IBView();
			inCmdList->IASetVertexBuffers(0, 1, &vbView);
			inCmdList->IASetIndexBuffer(&ibView);

			inCmdList->DrawIndexedInstanced(modelChild->IndexBuffer->IndexCount, 1, 0, 0, 0);

			//++NrMeshesDrawn;
		}
	}
}


void ShadowPass::Execute(ID3D12GraphicsCommandList* inCmdList, const glm::vec3& inLightDir)
{
	PIXMarker Marker(inCmdList, "Draw Depth Pass");

	// Build shadow matrix
	SceneManager& sManager = SceneManager::Get();
	const Scene& currentScene = sManager.GetCurrentScene();

	const eastl::shared_ptr<Camera>& currentCamera = currentScene.GetCurrentCamera();
	const glm::vec3 cameraPosW = currentCamera->GetAbsoluteTransform().Translation;
	const glm::vec3 shadowPosW = cameraPosW + (5.f * (-inLightDir));

	const glm::vec3 z_axis = glm::normalize(inLightDir);
	const glm::vec3 x_axis = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), z_axis));
	const glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis));

	const glm::mat4 shadowMatrix(
		glm::vec4(x_axis, 0),
		glm::vec4(y_axis, 0),
		glm::vec4(z_axis, 0),
		glm::vec4(0, 0, 0, 1));


	D3D12Utility::TransitionResource(inCmdList, ShadowDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	inCmdList->ClearDepthStencilView(ShadowDepthBuffer->DSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);


	D3D12Globals::GlobalConstantsBuffer.ClearUsedMemory();

	// Populate Command List

	inCmdList->SetGraphicsRootSignature(m_ShadowPassRootSignature);
	inCmdList->SetPipelineState(m_ShadowPassPSO);

	static D3D12_VIEWPORT viewport;
	viewport.Width = static_cast<float>(1024);
	viewport.Height = static_cast<float>(1024);
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	D3D12Globals::GraphicsCmdList->RSSetViewports(1, &viewport);

	// Handle RTs
	inCmdList->OMSetRenderTargets(0, nullptr, false, &ShadowDepthBuffer->DSV);


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
		DrawMeshNodesRecursively(inCmdList, children, currentScene, currModel->Materials, glm::transpose(shadowMatrix));
	}

	D3D12Utility::TransitionResource(inCmdList, ShadowDepthBuffer->Texture->Resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	viewport.Width = static_cast<float>(props.Width);
	viewport.Height = static_cast<float>(props.Height);
	D3D12Globals::GraphicsCmdList->RSSetViewports(1, &viewport);
}
