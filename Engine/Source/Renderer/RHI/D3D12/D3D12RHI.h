#pragma once
#include "Renderer/RHI/RHITypes.h"
#include "EASTL/string.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include "D3D12GraphicsTypes_Internal.h"
#include "dxcapi.h"

namespace D3D12Globals
{
	extern ID3D12Device* Device;
	extern IDXGISwapChain3* SwapChain;
	extern ID3D12CommandQueue* GraphicsCommandQueue;

	// Descriptor Heaps
	// TODO: Implement non-shader visible descriptor heaps that will be copied over into main heap when drawing
	extern D3D12DescriptorHeap GlobalRTVHeap;
	extern D3D12DescriptorHeap GlobalSRVHeap;
	extern D3D12DescriptorHeap GlobalDSVHeap;
}

struct GraphicsCompiledShaderPair
{
	IDxcBlob* VSByteCode = nullptr;
	IDxcBlob* PSByteCode = nullptr;

	GraphicsCompiledShaderPair(IDxcBlob* inVSByteCode, IDxcBlob* inPSByteCode)
		: VSByteCode(inVSByteCode), PSByteCode(inPSByteCode) {}

	~GraphicsCompiledShaderPair()
	{
		if (VSByteCode != nullptr)
		{
			VSByteCode->Release();
			VSByteCode = nullptr;
		}

		if (PSByteCode != nullptr)
		{
			PSByteCode->Release();
			PSByteCode = nullptr;
		}
	}

	GraphicsCompiledShaderPair(GraphicsCompiledShaderPair&& inOther) noexcept
	{
		this->VSByteCode = inOther.VSByteCode;
		this->PSByteCode = inOther.PSByteCode;

		inOther.VSByteCode = nullptr;
		inOther.PSByteCode = nullptr;
	}
};

class D3D12RHI
{
public:
	D3D12RHI() = default;
	~D3D12RHI();

	static void Init();
	static void Terminate();

	void InitPipeline();

	void EndFrame();

	void ImGuiBeginFrame();


	void ImGuiRenderDrawData();


	eastl::shared_ptr<class D3D12IndexBuffer> CreateIndexBuffer(const uint32_t* inData, uint32_t inCount) ;

	eastl::shared_ptr<class D3D12VertexBuffer> CreateVertexBuffer(const class VertexInputLayout& inLayout, const float* inVertices, const int32_t inCount, eastl::shared_ptr<class D3D12IndexBuffer> inIndexBuffer = nullptr) ;

	eastl::shared_ptr<class D3D12Texture2D> CreateAndLoadTexture2D(const eastl::string& inDataPath, const bool inSRGB, struct ID3D12GraphicsCommandList* inCommandList);

	eastl::shared_ptr<class D3D12RenderTarget2D> CreateRenderTexture(const int32_t inWidth, const int32_t inHeight, const eastl::wstring& inName, const ERHITexturePrecision inPrecision = ERHITexturePrecision::UnsignedByte,
		const ETextureState inInitialState = ETextureState::Render_Target, const ERHITextureFilter inFilter = ERHITextureFilter::Linear);

	eastl::shared_ptr<class D3D12DepthBuffer> CreateDepthBuffer(const int32_t inWidth, const int32_t inHeight, const eastl::wstring& inName, const ETextureState inInitialState = ETextureState::Render_Target);

	struct ID3D12RootSignature* CreateRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC& inDesc);
	GraphicsCompiledShaderPair CompileGraphicsShaderFromFile(const eastl::string& inFilePath);



	void ProcessDeferredReleases();


	static D3D12RHI* Get() { return Instance; }



private:
	eastl::vector<struct ID3D12Resource*> DeferredReleaseResources;

	inline static class D3D12RHI* Instance = nullptr;

};