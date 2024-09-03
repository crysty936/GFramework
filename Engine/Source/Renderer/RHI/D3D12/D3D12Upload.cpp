#include "D3D12Upload.h"
#include <d3d12.h>
#include "D3D12GraphicsTypes_Internal.h"
#include "D3D12Utility.h"
#include "Utils/Utils.h"
#include "D3D12RHI.h"

namespace D3D12Upload
{
	struct D3D12UploadQueue
	{
		ID3D12CommandQueue* D3D12Queue = nullptr;
		D3D12Fence Fence;
		uint64_t FenceValue = 0;

		//uint64_t WaitCount = 0;

		void Init(const wchar_t* inName)
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

			DXAssert(D3D12Globals::Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D3D12Queue)));
			D3D12Queue->SetName(inName);

			Fence.Init(0);

		}

		void SyncQueues(ID3D12CommandQueue* otherQueue)
		{
			// Force the other queue to wait for this queue to finish
			otherQueue->Wait(Fence.FenceHandle, FenceValue);
		}

		uint64_t SubmitCmdList(ID3D12CommandList* inCmdList)
		{
			ID3D12CommandList* cmdLists[1] = { inCmdList };
			D3D12Queue->ExecuteCommandLists(1, cmdLists);

			Fence.Signal(D3D12Queue, ++FenceValue);

			return FenceValue;
		}


		void Flush()
		{
			Fence.Wait(FenceValue);
		}


	};

	// Stores the Cmdlist(with its required allocator) and the necessary data in the ring buffer for submitting the upload command
	struct UploadSubmission
	{
		ID3D12CommandAllocator* CmdAllocator = nullptr;
		ID3D12GraphicsCommandList* CmdList = nullptr;

		// The offset from the start of the buffer to the available memory
		uint64_t Offset = 0;

		// The upload content size
		uint64_t Size = 0;
		uint64_t FenceValue = 0;

		// Currently only used to pad towards buffer end when wrapping around
		// Non-zero if submission wraps around, 0 otherwise
		uint64_t BufferEndPadding = 0;

		void Reset()
		{
			Offset = 0;
			Size = 0;
			FenceValue = 0;
			BufferEndPadding = 0;
		}

	};

	// Houses a ring buffer of Cmd lists and memory.
	// Elements in the buffer are removed in FIFO order and the buffer usage wraps around to allow using the most recent freed memory
	// All Cmd lists house the commands necessary for 1 upload request, 1 submission
	// Commands in the Cmd lists are added by the asking code
	// There is a 1:1 relationship between uploads and submissions, an upload request results in a commandlist
	// having commands for a request added and being submitted. Optimal for uploading big chunks of memory(textures), suboptimal for small and numerous requests
	struct UploadRingBuffer
	{
		static const uint64_t MaxSubmissions = 16;
		UploadSubmission Submissions[MaxSubmissions];

		uint64_t SubmissionStart = 0;
		uint64_t SubmissionsUsed = 0;

		uint64_t BufferSize = 64 * 1024 * 1024;
		ID3D12Resource* D3DBuffer = nullptr;
		uint8_t* BufferCPUAddress = nullptr;

		uint64_t BufferStart = 0;
		uint64_t BufferUsed = 0;

		D3D12UploadQueue* UsedUploadQueue = nullptr;

		void Init(D3D12UploadQueue* inUploadQueue)
		{
			UsedUploadQueue = inUploadQueue;

			for (uint64_t i = 0; i < MaxSubmissions; ++i)
			{
				UploadSubmission& currentSubmission = Submissions[i];

				DXAssert(D3D12Globals::Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&currentSubmission.CmdAllocator)));
				DXAssert(D3D12Globals::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, currentSubmission.CmdAllocator, nullptr, IID_PPV_ARGS(&currentSubmission.CmdList)));

				DXAssert(currentSubmission.CmdList->Close());

				currentSubmission.CmdList->SetName(L"Upload Queue Command List");
			}

			Resize(BufferSize);
		}

		void Resize(const uint64_t inNewSize)
		{
			if (D3DBuffer)
			{
				D3DBuffer->Release();
				D3DBuffer = nullptr;
			}

			BufferSize = inNewSize;

			D3D12_RESOURCE_DESC resourceDesc = {};

			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = BufferSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Alignment = 0;

			DXAssert(D3D12Globals::Device->CreateCommittedResource(&D3D12Utility::GetUploadHeapProps(), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&D3DBuffer)));
			D3DBuffer->SetName(L"Main Upload Ring Buffer");

			D3D12_RANGE readRange = {};

			DXAssert(D3DBuffer->Map(0, &readRange, reinterpret_cast<void**>(&BufferCPUAddress)));
		}


		uint64_t AllocSubmission(uint64_t inUploadSize)
		{
			ASSERT(SubmissionsUsed <= MaxSubmissions);

			if (SubmissionsUsed == MaxSubmissions)
			{
				// All cmd lists are taken up
				return uint64_t(-1);
			}

			if (inUploadSize > (BufferSize - BufferUsed))
			{
				// Buffer full
				return uint64_t(-1);
			}

			const uint64_t submissionIdx = (SubmissionStart + SubmissionsUsed) % MaxSubmissions;

			const uint64_t start = BufferStart;
			const uint64_t end = BufferStart + BufferUsed;
			
			uint64_t allocOffsetFromStart = uint64_t(-1);
			uint64_t paddingForWrap = 0;

			if (end < BufferSize)
			{
				const uint64_t availableTowardsEnd = BufferSize - end;
				if (availableTowardsEnd >= inUploadSize)
				{
					// We have enough space towards the end of the buffer
					allocOffsetFromStart = end;
				}
				else if (start >= inUploadSize)
				{
					// We don't have enough space at the end of the buffer but we have enough starting from the beginning
					// Wrap around
					allocOffsetFromStart = 0;
					BufferUsed += availableTowardsEnd; //Becase we wrap around, include space towards end as used by this submission
					paddingForWrap = availableTowardsEnd;
				}
			}
			else
			{
				// We are already wrapped around, calculate if enough size is left towards start

				const uint64_t wrappedEnd = end % BufferSize;
				if ((start - wrappedEnd) >= inUploadSize)
				{
					// Enough space left between the wrapped end and start
					allocOffsetFromStart = wrappedEnd;
				}
			}

			if (allocOffsetFromStart == uint64_t(-1))
			{
				return uint64_t(-1);
			}

			++SubmissionsUsed;
			BufferUsed += inUploadSize; // This already includes any padding that may already have been added for this submission

			UploadSubmission& currentSubmission = Submissions[submissionIdx];
			currentSubmission.Offset = allocOffsetFromStart;
			currentSubmission.Size = inUploadSize;
			currentSubmission.FenceValue = uint64_t(-1);
			currentSubmission.BufferEndPadding = paddingForWrap;

			return submissionIdx;
		}

		void ClearUploads(uint64_t inForceWaitCount)
		{
			const uint64_t start = SubmissionStart;
			const uint64_t used = SubmissionsUsed;

			for (uint64_t i = 0; i < used; ++i)
			{
				const uint64_t idx = (start + i) % MaxSubmissions;
				UploadSubmission& currSubmission = Submissions[idx];

				// This can be allowed to be -1 only if using multithreading and submitting upload requests on different threads
				// Otherwise, it's because ResourceEndUpload has not been called somewhere
				ASSERT(currSubmission.FenceValue != uint64_t(-1));

				ID3D12Fence* submitFence = UsedUploadQueue->Fence.FenceHandle;

				uint64_t uploadQueueFenceValue = submitFence->GetCompletedValue();

				if (i < inForceWaitCount && uploadQueueFenceValue < currSubmission.FenceValue)
				{
					// This does not return until FenceValue has been reached
					submitFence->SetEventOnCompletion(currSubmission.FenceValue, nullptr);
				}

				uploadQueueFenceValue = submitFence->GetCompletedValue();

				if (uploadQueueFenceValue >= currSubmission.FenceValue)
				{
					// Advance start position

					SubmissionStart = (SubmissionStart + 1) % MaxSubmissions;
					--SubmissionsUsed;
					
					// The current submission should always be the first submission in the ring buffer
					ASSERT(currSubmission.Offset == (BufferStart + currSubmission.BufferEndPadding));
					ASSERT((BufferStart + currSubmission.Size) <= BufferSize);

					BufferStart = (BufferStart + currSubmission.Size + currSubmission.BufferEndPadding) % BufferSize;
					BufferUsed -= currSubmission.Size + currSubmission.BufferEndPadding;

					currSubmission.Reset();

					if (BufferUsed == 0)
					{
						// Reset to beginning of actual buffer
						BufferStart = 0;
					}
				}
				else
				{
					// Retire submissions only in order, even if others are done before
					break;
				}


			}



		}

		void Flush()
		{
			ClearUploads(uint64_t(-1));
		}

		UploadContext Begin(const uint64_t inUploadSize)
		{

			ASSERT(inUploadSize > 0);

			const uint64_t alignedSize = Utils::AlignTo(inUploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);


			if (alignedSize > BufferSize)
			{
				// Ensure all command lists are finished
				Flush();

				Resize(alignedSize);
			}

			uint64_t newSubmissionIdx = uint64_t(-1);

			// Get a free submission from the pool
			{
				ClearUploads(0);

				newSubmissionIdx = AllocSubmission(alignedSize);

				if (newSubmissionIdx == uint64_t(-1))
				{
					// Clear at least one submission and retry
					ClearUploads(1);
					newSubmissionIdx = AllocSubmission(alignedSize);

					// Should not be invalid now
					ASSERT(newSubmissionIdx != uint64_t(-1));
				}
			}

			UploadSubmission& newSubmission = Submissions[newSubmissionIdx];

			DXAssert(newSubmission.CmdAllocator->Reset());
			DXAssert(newSubmission.CmdList->Reset(newSubmission.CmdAllocator, nullptr));

			UploadContext newContext;
			newContext.CmdList = newSubmission.CmdList;
			newContext.Resource = D3DBuffer;
			newContext.CPUAddress = BufferCPUAddress + newSubmission.Offset;
			newContext.ResourceOffset = newSubmission.Offset;
			newContext.SubmissionIndex = newSubmissionIdx;

			return newContext;
		}

		uint64_t End(UploadContext& inContext)
		{
			ASSERT(inContext.SubmissionIndex != uint64_t(-1));

			UploadSubmission& currSubmission = Submissions[inContext.SubmissionIndex];

			DXAssert(currSubmission.CmdList->Close());
			currSubmission.FenceValue = UsedUploadQueue->SubmitCmdList(currSubmission.CmdList);

			inContext = UploadContext();

			return currSubmission.FenceValue;
		}


	};
	


	D3D12UploadQueue UploadQueue;
	UploadRingBuffer RingBuffer;


	void InitUpload()
	{
		UploadQueue.Init(L"Upload Queue");
		RingBuffer.Init(&UploadQueue);




	}

	void EndFrame()
	{
		// TODO
		// This is basically equivalent to flushing the ring buffer
		// The wait should not be done for all requests in the queue but checking for individual assets when they are needed, 
		// if they are done based on their submission Fence Value
		//RingBuffer.Flush();
	 	UploadQueue.SyncQueues(D3D12Globals::GraphicsCommandQueue);

	}

	UploadContext ResourceUploadBegin(const uint64_t inSize)
	{
		return RingBuffer.Begin(inSize);
	}

	uint64_t ResourceUploadEnd(UploadContext& inContext)
	{
		return RingBuffer.End(inContext);
	}



}

