#pragma once
#include <d3d12.h>
#include <stdint.h>

struct UploadContext
{
	ID3D12GraphicsCommandList* CmdList = nullptr;
	void* CPUAddress = nullptr;
	uint64_t ResourceOffset = 0;
	ID3D12Resource* Resource = nullptr;

	// For internal usage by the ring buffer
	uint64_t SubmissionIndex = uint64_t(-1);
};

namespace D3D12Upload
{
	void InitUpload();
	void EndFrame();

	UploadContext ResourceUploadBegin(const uint64_t inSize);
	uint64_t ResourceUploadEnd(UploadContext& inContext);

}