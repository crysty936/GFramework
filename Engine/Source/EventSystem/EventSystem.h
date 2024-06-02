#pragma once
#include <type_traits>
#include "EASTL/vector.h"
#include "DelegateBase.h"
#include "EventSystem/IFunctionContainerBase.h"
#include "Core/EngineUtils.h"

template<typename inRetType, typename... inParamTypes>
class IFunctionContainer : IFunctionContainerBase
{
public:
	virtual inRetType Execute(inParamTypes&&...) const = 0;
	virtual bool IsBound() const = 0;
	virtual ~IFunctionContainer() = default;
};

template<typename inRetType, typename... inParamTypes>
class FreeFunctionContainer : public IFunctionContainer<inRetType, inParamTypes...>
{
	using FreeFunctionType = inRetType(*)(inParamTypes...);

public:
	FreeFunctionContainer(FreeFunctionType inFunc) noexcept
		: Func{ inFunc } 
	{}

	inRetType Execute(inParamTypes&&... inParams) const
	{
		return Func(std::forward<inParamTypes>(inParams)...);
	}

	FreeFunctionType Func;
};

template<bool isConst, typename inObjType, typename inRetType, typename... inParamTypes>
struct MemberFuncPtrType;

template <typename inObjType, typename inRetType, typename... inParamTypes>
struct MemberFuncPtrType<false, inObjType, inRetType, inParamTypes...>
{
	using Type = inRetType(inObjType::*)(inParamTypes...);
};

template <typename inObjType, typename inRetType, typename... inParamTypes>
struct MemberFuncPtrType<true, inObjType, inRetType, inParamTypes...>
{
	using Type = inRetType(inObjType::*)(inParamTypes...) const;
};

template<bool isConst, typename inObjType, typename inRetType, typename... inParamTypes>
class MemberFuncContainer : public IFunctionContainer<inRetType, inParamTypes...>
{
	using MemberFunctionType = typename MemberFuncPtrType<isConst, inObjType, inRetType, inParamTypes...>::Type;

public:
	MemberFuncContainer(inObjType* inObj, MemberFunctionType inMemberFunction) noexcept
		:Obj(inObj), MemberFunctionPtr(inMemberFunction)
	{}

	virtual ~MemberFuncContainer() = default;

	inRetType Execute(inParamTypes&&... inParams) const
	{
		ASSERT(Obj);

		return (Obj->*MemberFunctionPtr)(std::forward<inParamTypes>(inParams)...);
	}


	virtual bool IsBound() const override
	{
		return !!Obj;
	}

private:
	inObjType* Obj = nullptr;
	MemberFunctionType MemberFunctionPtr = nullptr;
};


template<typename inRetType, typename... inParamTypes>
class Delegate : public DelegateBase
{
	template<bool isConst, typename inObjType>
	using MemberFunc = typename MemberFuncPtrType<isConst, inObjType, inRetType, inParamTypes...>::Type;

	template<bool isConst, typename inObjType>
	using MemberFuncContainerType = MemberFuncContainer<isConst, inObjType, inRetType, inParamTypes...>;

	using FreeFuncContainerType = FreeFunctionContainer<inRetType, inParamTypes...>;
	using FreeFunctionType = inRetType(*)(inParamTypes...);
	using FuncContainerType = IFunctionContainer<inRetType, inParamTypes...>;

public:
	Delegate() = default;
	~Delegate() = default;
	Delegate(const Delegate& inOther) = default;
	Delegate& operator=(const Delegate& inOther) = default;
	Delegate(Delegate&& inOther) = default;
	Delegate& operator=(Delegate&& inOther) = default;

	static Delegate CreateStatic(FreeFunctionType inFunc)
	{
		Delegate del;

		new(del) FreeFuncContainerType(inFunc);

		return del;
	}

	void BindStatic(FreeFunctionType inFunc)
	{
		*this = CreateStatic(inFunc);
	}

	template<typename inObjType>
	static Delegate CreateRaw(inObjType* inObj, MemberFunc<false, inObjType> inMemberFunction)
	{
		Delegate del;

		new(del) MemberFuncContainerType<false, inObjType>(inObj, inMemberFunction);

		return del;
	}

	template<typename inObjType>
	static Delegate CreateRaw(const inObjType* inObj, MemberFunc<true, inObjType> inMemberFunction)
	{
		Delegate del;

		new(del) MemberFuncContainerType<true, const inObjType>(inObj, inMemberFunction);

		return del;
	}

	template<typename inObjType>
	void BindRaw(inObjType* inObj, MemberFunc<false, inObjType> inMemberFunction)
 	{
		static_assert(!std::is_const<inObjType>::value, "Attempting to bind a const object with a non const member Function.");

		*this = CreateRaw(inObj, inMemberFunction);
	}

 	template<typename inObjType>
 	void BindRaw(const inObjType* inObj, MemberFunc<true, inObjType> inMemberFunction)
 	{
		*this = CreateRaw(inObj, inMemberFunction);
 	}

	inRetType Execute(inParamTypes... inParams) const
	{
		const FuncContainerType* funcContainer = GetFuncContainer();

		return funcContainer->Execute(std::forward<inParamTypes>(inParams)...);
	}

	inline bool IsBound() const
	{
		return GetFuncContainer() ? GetFuncContainer()->IsBound() : false;
	}

private:
	const FuncContainerType* GetFuncContainer() const
	{
		const IFunctionContainerBase* funcContainerBase = GetDelegateInstance();
		const FuncContainerType* funcContainer = static_cast<const FuncContainerType*>(funcContainerBase);

		return funcContainer;
	}
};

 template<typename... inParamTypes>
 class MulticastDelegate
 {
 	using RetType = void;
 	using DelegateType = Delegate<RetType, inParamTypes...>;

	template<bool isConst, typename inObjType>
	using MemberFunc = typename MemberFuncPtrType<isConst, inObjType, RetType, inParamTypes...>::Type;

	template<bool isConst, typename inObjType>
	using MemberFuncContainerType = MemberFuncContainer<isConst, inObjType, RetType, inParamTypes...>;

	using FreeFuncContainerType = FreeFunctionContainer<RetType, inParamTypes...>;
	using FreeFunctionType = RetType(*)(inParamTypes...);

	using BaseFuncContainerType = IFunctionContainer<RetType, inParamTypes...>;  

 public:
 	MulticastDelegate() = default;
	~MulticastDelegate() = default;
	MulticastDelegate(const MulticastDelegate& inOther) = default;
	MulticastDelegate& operator=(const MulticastDelegate& inOther) = default;
	MulticastDelegate(MulticastDelegate&& inOther) = default;
	MulticastDelegate& operator=(MulticastDelegate&& inOther) = default;

	void BindStatic(FreeFunctionType inFunc)
	{
		DelegateType del;

		new(del) FreeFuncContainerType(inFunc);

		Delegates.push_back(del);
	}

	template<typename inObjType>
	void BindRaw(inObjType* inObj, MemberFunc<false, inObjType> inMemberFunction)
	{
		DelegateType del;

		new(del) MemberFuncContainerType<false, inObjType>(inObj, inMemberFunction);

		Delegates.push_back(del);
	}

 	template<typename inObjType>
 	void BindRaw(const inObjType* inObj, MemberFunc<true, inObjType> inMemberFunction)
 	{
 		DelegateType del;
 
 		new(del) MemberFuncContainerType<true, inObjType>(inObj, inMemberFunction);
 
 		Delegates.push_back(del);
 	}

 	void Invoke(inParamTypes... inParams)
 	{
  		for (const DelegateType& del : Delegates)
  		{
  			del.Execute(std::forward<inParamTypes>(inParams)...);
  		}
 	}
 
	size_t GetAllocationsSize()
	{
		size_t sum = 0;
		for (int32_t i = 0; i < Delegates.size(); i++)
		{
			sum += Delegates[i].GetAllocatedSize();
		}

		return sum;
	}

 private:
 	eastl::vector<DelegateType> Delegates{};
 };
