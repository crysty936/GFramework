#pragma once
#include "Renderer/RHI/RHI.h"
#include "EASTL/string.h"

class D3D12RHI
{
public:
	D3D12RHI();
	~D3D12RHI();

	static void Init();
	static void Terminate();

	void Init_Internal() ;

	void WaitForPreviousFrame();
	void MoveToNextFrame();
	void Test() ;

	void ImGuiInit() ;

	void ImGuiBeginFrame() ;


	void ImGuiRenderDrawData() ;


	void SwapBuffers() ;

	eastl::shared_ptr<class D3D12IndexBuffer> CreateIndexBuffer(const uint32_t* inData, uint32_t inCount) ;

	eastl::shared_ptr<class D3D12VertexBuffer> CreateVertexBuffer(const class VertexInputLayout& inLayout, const float* inVertices, const int32_t inCount, eastl::shared_ptr<class D3D12IndexBuffer> inIndexBuffer = nullptr) ;




	eastl::shared_ptr<class D3D12Texture2D> CreateAndLoadTexture2D(const eastl::string& inDataPath, const bool inSRGB) ;



	eastl::shared_ptr<class D3D12RenderTarget2D> CreateRenderTexture(const int32_t inWidth, const int32_t inHeight, const ERHITexturePrecision inPrecision = ERHITexturePrecision::UnsignedByte, const ERHITextureFilter inFilter = ERHITextureFilter::Linear) ;

	void BeginFrame();

	static D3D12RHI* Get() { return Instance; }

private:
	inline static class D3D12RHI* Instance = nullptr;
	bool bCmdListOpen = false;


};