#include "EventSystem/DelegateBase.h"

DelegateBase::DelegateBase() = default;
DelegateBase::~DelegateBase() = default;

DelegateBase& DelegateBase::operator=(const DelegateBase& inOther) = default;
DelegateBase::DelegateBase(const DelegateBase& inOther) = default;

DelegateBase::DelegateBase(DelegateBase&& inOther) = default;
DelegateBase& DelegateBase::operator=(DelegateBase&& inOther) = default;

void DelegateBase::Unbind()
{
	if (const IFunctionContainerBase* currentInstance = GetDelegateInstance())
	{
		currentInstance->~IFunctionContainerBase();

		Allocator.Free();
	}
}

void* DelegateBase::Allocate(size_t inSize)
{
	if (const IFunctionContainerBase* currentInstance = GetDelegateInstance())
	{
		currentInstance->~IFunctionContainerBase();
	}

	return Allocator.Allocate(inSize);
}

