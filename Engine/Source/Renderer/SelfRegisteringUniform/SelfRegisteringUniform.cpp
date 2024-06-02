#include "SelfRegisteringUniform.h"
#include "Renderer/RHI/Resources/UniformBufferContainer.h"

SelfRegisteringUniform::SelfRegisteringUniform(const int32_t inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<int32_t>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const uint32_t inValue)
	: Data{eastl::make_unique<SelfRegisteringUniformData<uint32_t>>(inValue)} {}

SelfRegisteringUniform::SelfRegisteringUniform(const float inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<float>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const glm::mat4& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<glm::mat4>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const glm::vec2& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<glm::vec2>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const glm::vec3& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<glm::vec3>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const glm::vec4& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<glm::vec4>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform()
	{}

SelfRegisteringUniform::SelfRegisteringUniform(const eastl::vector<float>& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<eastl::vector<float>>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const eastl::vector<glm::vec4>& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<eastl::vector<glm::vec4>>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const eastl::vector<glm::mat4>& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<eastl::vector<glm::mat4>>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const eastl::vector<SPointLight>& inValue)
	: Data{ eastl::make_unique<SelfRegisteringUniformData<eastl::vector<SPointLight>>>(inValue) } {}

SelfRegisteringUniform::SelfRegisteringUniform(const eastl::shared_ptr<RHITexture2D>& inValue)
	: Data{ eastl::make_unique < SelfRegisteringUniformData<eastl::shared_ptr<RHITexture2D>>>(inValue) } {}


void SelfRegisteringUniform::Register(UniformBufferContainer& inBuffer, const size_t inRequiredCount) const
{
	Data->SelfRegister(inBuffer, inRequiredCount);
}

