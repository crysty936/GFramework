#pragma once
#include "EASTL/allocator.h"
#include "EASTL/vector.h"
#include "Core/EngineUtils.h"

// TODO: Needs more work, eastl::vector swaps the allocators as well as mpBegin and mpEnd when moving and that leaves the new vector with pointers to the previous stack segment

template<typename T, uint32_t NumInlineElements>
class TFixedInlineAllocator
{
public:
	TFixedInlineAllocator(const char* pName = "FixedInlineAllocator");
 	TFixedInlineAllocator(const TFixedInlineAllocator& x);
 	TFixedInlineAllocator(const TFixedInlineAllocator& x, const char* pName);
 
 	TFixedInlineAllocator& operator=(const TFixedInlineAllocator& x);

	void* allocate(size_t n, int flags = 0);
	void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
	void  deallocate(void* p, size_t n);

	//const char* get_name() const;
	//void        set_name(const char* pName);

	static eastl_size_t GetNewCapacity(eastl_size_t currentCapacity);

protected:
	T InlineElements[NumInlineElements];
	//const char* mpName; // Debug name, used to track memory.
private:
	void CopyAllocator(const TFixedInlineAllocator<T, NumInlineElements>& inAlloc);
};

template<typename T, uint32_t NumInlineElements>
void TFixedInlineAllocator<T, NumInlineElements>::CopyAllocator(const TFixedInlineAllocator<T, NumInlineElements>& inAlloc)
{
	for (int i = 0; i < NumInlineElements; ++i)
	{
		InlineElements[i] = inAlloc.InlineElements[i];
	}
}

template<typename T, uint32_t NumInlineElements>
TFixedInlineAllocator<T, NumInlineElements>::TFixedInlineAllocator(const char* pName /*= "FixedInlineAllocator"*/)
{

}

template<typename T, uint32_t NumInlineElements>
TFixedInlineAllocator<T, NumInlineElements>::TFixedInlineAllocator(const TFixedInlineAllocator<T, NumInlineElements>& x, const char* pName)
{
	CopyAllocator(x);
}

template<typename T, uint32_t NumInlineElements>
TFixedInlineAllocator<T, NumInlineElements>::TFixedInlineAllocator(const TFixedInlineAllocator<T, NumInlineElements>& x)
{
	CopyAllocator(x);
}


template<typename T, uint32_t NumInlineElements>
TFixedInlineAllocator<T, NumInlineElements>& TFixedInlineAllocator<T, NumInlineElements>::operator=(const TFixedInlineAllocator<T, NumInlineElements>& x)
{
	CopyAllocator(x);

	return *this;
}

template<typename T, uint32_t NumInlineElements>
inline void* TFixedInlineAllocator<T, NumInlineElements>::allocate(size_t n, int flags)
{
	ASSERT_MSG(n <= sizeof(T)* NumInlineElements, "Fixed inline array used with more elements than declared, will crash.");

	return &InlineElements;
}


template<typename T, uint32_t NumInlineElements>
inline void* TFixedInlineAllocator<T, NumInlineElements>::allocate(size_t n, size_t alignment, size_t offset, int flags)
{
	return nullptr;
}


template<typename T, uint32_t NumInlineElements>
inline void TFixedInlineAllocator<T, NumInlineElements>::deallocate(void* p, size_t)
{
}

template<typename T, uint32_t NumInlineElements>
inline eastl_size_t TFixedInlineAllocator<T, NumInlineElements>::GetNewCapacity(eastl_size_t currentCapacity)
{
	return currentCapacity + 1;
}

template<typename T, uint32_t NumInlineElements>
using vectorInline = eastl::vector<T, TFixedInlineAllocator<T, NumInlineElements>>;
