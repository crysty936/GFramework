#pragma once
#include <stdint.h>
#include "EASTL/allocator.h"
#include "EASTL/vector.h"

/**
 * Allocator that allocates inline up to a point, from which it allocates on the heap.
 */

constexpr int StackSize = 32;

class InlineAllocator
{
public:
	InlineAllocator();
	~InlineAllocator();

	InlineAllocator(const InlineAllocator& inOther);
	InlineAllocator& operator=(const InlineAllocator& inOther);

	InlineAllocator(InlineAllocator&& inOther);
	InlineAllocator& operator=(InlineAllocator&& inOther);

	void* Allocate(const size_t inSize);

	const void* GetAllocation() const;

	void Free();

	bool HasAllocation () const { return AllocatedSize > 0; }

	bool HasHeapAllocation () const { return AllocatedSize > StackSize; }

	inline size_t GetAllocatedSize() const { return AllocatedSize; }

private:
	size_t AllocatedSize = 0;

	union
	{
		uint8_t StackPtr[StackSize];
		void* HeapPtr = nullptr;
	};
};
