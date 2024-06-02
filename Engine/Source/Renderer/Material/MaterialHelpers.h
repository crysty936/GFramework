#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "EASTL/shared_ptr.h"
#include "Renderer/RHI/Resources/UniformBufferContainer.h"
#include "EASTL/unordered_map.h"
#include "Renderer/RHI/Resources/RHIBuffers.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"

class MaterialHelpers
{
public:
	template<typename T>
	static bool TryGetUniformFromCache(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const eastl::string& inUniformName, T& outUniform);

private:
	template<typename T>
	static T& GetUniformValue(SelfRegisteringUniform& inUniform);
};

template<typename T>
bool MaterialHelpers::TryGetUniformFromCache(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const eastl::string& inUniformName, T& outUniform)
{
	auto& iter = inUniformsCache.find(inUniformName);
	 
	if (iter == inUniformsCache.end())
	{
	 	return false;
	}
	 
	outUniform = (*iter).second.GetValue<T>();

	return true;
}
