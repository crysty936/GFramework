#include "RenderMaterial.h"
#include "Renderer/SelfRegisteringUniform/SelfRegisteringUniform.h"
#include "Core/EngineUtils.h"

RenderMaterial::RenderMaterial() = default;
RenderMaterial::~RenderMaterial() = default;

void RenderMaterial::Init()
{
	SetRequiredUniforms();
}

void RenderMaterial::ResetUniforms()
{
	for (BufferWithRequirements& buffer : UBuffers)
	{
		for (UniformWithFlag& u : buffer.RequiredUniforms)
		{
			u.IsSet = false;
		}
	}

}

void RenderMaterial::SetRequiredUniforms()
{
	// !! These have to be in the same order as they are in the shader struct
	eastl::vector<UniformWithFlag> defaultUniforms = {
		{"projection"},
		{"view"},
		{"model"}
		};

	UBuffers.push_back({ defaultUniforms, EShaderType::Sh_Vertex });
}

void RenderMaterial::SetUniformsValue(eastl::unordered_map<eastl::string, SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderType)
{
	using uniformsIterator = const eastl::unordered_map<eastl::string, SelfRegisteringUniform>::const_iterator;

	for (int32_t i = 0; i < UBuffers.size(); ++i)
	{
		BufferWithRequirements& buffer = UBuffers[i];
		if (!(buffer.BufferBindingType & inShaderType))
		{
			continue;
		}

		buffer.BufferContainer.Clear();

		// Register all required uniforms
		for (UniformWithFlag& requiredUniform : buffer.RequiredUniforms)
		{
			//test code
// 			if (requiredUniform.UniformName == "PointLights")
// 			{
// 				__debugbreak();
// 			}
			//

			if (requiredUniform.IsSet)
			{
				continue;
			}

			const uniformsIterator& iter = inUniformsCache.find(requiredUniform.UniformName);

			if (iter == inUniformsCache.end())
			{
				ENSURE_MSG(false, "Failed to find uniform with name %s in cache.", requiredUniform.UniformName.c_str());
				continue;
			}

			const SelfRegisteringUniform& selfRegisteringUniform = (*iter).second;
			selfRegisteringUniform.Register(buffer.BufferContainer, requiredUniform.RequiredCount);
			requiredUniform.IsSet = true;
		}

		buffer.BufferContainer.UpdateData(buffer.BufferBindingType, i);
	}
}

void RenderMaterial::BindBuffers(const EShaderType inShaderTypes)
{
	for (BufferWithRequirements& buffer : UBuffers)
	{
		if (!(buffer.BufferBindingType & inShaderTypes))
		{
			continue;
		}

		buffer.BufferContainer.Bind();
	}
}

void RenderMaterial::UnbindBuffers(const EShaderType inShaderTypes)
{
	for (BufferWithRequirements& buffer : UBuffers)
	{
		if (!(buffer.BufferBindingType & inShaderTypes))
		{
			continue;
		}

		buffer.BufferContainer.Unbind();
	}
}

UniformWithFlag* RenderMaterial::FindRequiredUniform(const eastl::string& inUniformName)
{
// 	auto iterRequiredUniform = eastl::find_if(RequiredUniforms.begin(), RequiredUniforms.end(), [&](const UniformWithFlag& arg) {
// 		return arg.UniformName == inUniformName; });
// 
// 	if (iterRequiredUniform == RequiredUniforms.end())
// 	{
// 		return nullptr;
// 	}

	//return &*iterRequiredUniform;

	return nullptr;
}
