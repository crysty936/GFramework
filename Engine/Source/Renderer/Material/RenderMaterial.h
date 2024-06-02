#pragma once
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "EASTL/shared_ptr.h"
#include "Renderer/RHI/Resources/UniformBufferContainer.h"
#include "EASTL/unordered_map.h"
#include "Renderer/RHI/Resources/RHIBuffers.h"

struct UniformWithFlag
{
	UniformWithFlag(const eastl::string& inName, size_t inCount = 0) : UniformName{ inName }, RequiredCount{inCount} {}
	eastl::string UniformName;
	bool IsSet = false;
	size_t RequiredCount = 0; // For array members
};

struct BufferWithRequirements
{
	BufferWithRequirements(eastl::vector<UniformWithFlag>& inRequiredUniforms, EShaderType inBufferType)
		: RequiredUniforms(inRequiredUniforms), BufferBindingType(inBufferType), BufferContainer()
	{}

	eastl::vector<UniformWithFlag> RequiredUniforms;
	EShaderType BufferBindingType = EShaderType::Sh_Vertex;
	UniformBufferContainer BufferContainer;
};

class RenderMaterial
{
public:
	RenderMaterial();
	~RenderMaterial();

	virtual void Init();
	void ResetUniforms();
	virtual void SetRequiredUniforms();
	virtual void SetUniformsValue(eastl::unordered_map<eastl::string, struct SelfRegisteringUniform>& inUniformsCache, const EShaderType inShaderTypes = Sh_Universal);
	void BindBuffers(const EShaderType inShaderTypes = Sh_Universal);
	void UnbindBuffers(const EShaderType inShaderTypes = Sh_Universal);
	UniformWithFlag* FindRequiredUniform(const eastl::string& inUniformName);

public:
	eastl::vector<BufferWithRequirements> UBuffers;
	eastl::shared_ptr<class RHIShader> Shader;

	// Use for pre-stencil, initialized when it is required, otherwise NULL
	mutable eastl::shared_ptr<class RHIShader> VertexOnlyShader;

	eastl::vector<eastl::shared_ptr<class RHITexture2D>> OwnedTextures;
	eastl::vector<eastl::weak_ptr<class RHITexture2D>> ExternalTextures;

	bool bCastShadow = true;
	bool bUsesSceneTextures = false;
};