#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "Renderer/RHI/Resources/UniformBufferContainer.h"
#include "Core/EngineUtils.h"
#include "Renderer/RHI/Resources/RHITexture.h"
#include "Renderer/ShaderTypes.h"

class IUniformData
{
public:
	virtual void SelfRegister(UniformBufferContainer& inBufferContainer, const size_t inRequiredCount) = 0;

private:
	virtual void* GetData() = 0;
	/** Equal to GetElementSize for non array members*/
	virtual size_t GetSize() = 0;

	/** Equal to sizeof(T) */
	virtual size_t GetElementSize() = 0;
};

template<typename T>
class SelfRegisteringUniformData : public IUniformData
{
public:
	SelfRegisteringUniformData(const T& inData)
	{
		Data = inData;
	}

	virtual void* GetData() override
	{
		return &Data;
	}

	virtual size_t GetSize() override 
	{
		return GetElementSize();
	}

	virtual size_t GetElementSize() override
	{
		return sizeof(T);
	}

	virtual void SelfRegister(UniformBufferContainer& inBufferContainer, const size_t inRequiredCount) override
	{
		inBufferContainer.AddData(GetData(), GetSize());
	}

public:
	T Data;
};

// Vec2 specialization
template<>
class SelfRegisteringUniformData<glm::vec2> : public IUniformData
{
public:
	SelfRegisteringUniformData(const glm::vec2& inData)
	{
		Data = inData;
	}

	virtual void* GetData() override
	{
		AlignedData = glm::vec4(Data.x, Data.y, 0.f, 1.f);
		return &AlignedData;
	}

	virtual size_t GetSize() override
	{
		return GetElementSize();
	}

	virtual void SelfRegister(UniformBufferContainer& inBufferContainer, const size_t inRequiredCount) override
	{
		inBufferContainer.AddData(GetData(), GetSize());
	}

	virtual size_t GetElementSize() override
	{
		return sizeof(float) * 4; // Align to 16 bytes
	}

public:
	glm::vec2 Data;
	glm::vec4 AlignedData{};
};


// Vec3 specialization
template<>
class SelfRegisteringUniformData<glm::vec3> : public IUniformData
{
public:
	SelfRegisteringUniformData(const glm::vec3& inData)
	{
		Data = inData;
	}

	virtual void* GetData() override
	{
		AlignedData = glm::vec4(Data.x, Data.y, Data.z, 1.f);
		return &AlignedData;
	}

	virtual size_t GetSize() override
	{
		return GetElementSize();
	}

	virtual void SelfRegister(UniformBufferContainer& inBufferContainer, const size_t inRequiredCount) override
	{
		inBufferContainer.AddData(GetData(), GetSize());
	}

	virtual size_t GetElementSize() override
	{
		return sizeof(float) * 4; // Align to 16 bytes
	}

public:
	glm::vec3 Data;
	glm::vec4 AlignedData{};
};

template<typename T>
class SelfRegisteringUniformData<eastl::vector<T>> : public IUniformData
{
public:
	SelfRegisteringUniformData(const eastl::vector<T>& inData)
	{
		Data = inData;
	}

	virtual void* GetData() override
	{
		return Data.data();
	}

	virtual size_t GetSize() override
	{
		return GetElementSize() * Data.size();
	}

	virtual size_t GetElementSize() override
	{
		return sizeof(T);
	}

	virtual void SelfRegister(UniformBufferContainer& inBufferContainer, const size_t inRequiredCount) override
	{
		// Each element in an array has a base aligment of 4 * 4 = 16 bytes ( vec4 )

		ASSERT(Data.size() <= inRequiredCount);

		const int32_t paddingElementsCount = static_cast<int32_t>(inRequiredCount - Data.size());
		if (sizeof(T) < 16)
		{
			// Each element is smaller than the required number, add one by one and let the buffer container align them
			for (const T& Element : Data)
			{
 				//char newData[16];
 				//memcpy(&newData[16 - sizeof(T)], &Element, sizeof(T));
				//inBufferContainer.AddData(&newData, 16, false);

				inBufferContainer.AddData(&Element, GetElementSize(), true);
			}

			// Add empty to fill the array
			for (int32_t i = 0; i < paddingElementsCount; ++i)
			{
				inBufferContainer.AddData(nullptr, GetElementSize(), true);
			}
		}
		else
		{
			// Elements have the right alignment, add all of them at once
			inBufferContainer.AddData(GetData(), GetSize());

			// Add empty to fill the array
			for (int32_t i = 0; i < paddingElementsCount; ++i)
			{
				inBufferContainer.AddData(nullptr, GetElementSize());
			}
		}
	}

public:
	eastl::vector<T> Data;
};

// shared_ptr specialization
// These are supposed to be just data holders to be used as utilities in materials, not self-registering
template<typename T>
class SelfRegisteringUniformData<eastl::shared_ptr<T>> : public IUniformData
{
public:
	SelfRegisteringUniformData(const eastl::shared_ptr<T>& inData)
	{
		Data = inData;
	}

	virtual void* GetData() override
	{
		return nullptr;
	}

	virtual size_t GetSize() override
	{
		return GetElementSize();
	}

	virtual void SelfRegister(UniformBufferContainer& inBufferContainer, const size_t inRequiredCount) override
	{}

	virtual size_t GetElementSize() override
	{
		return sizeof(T);
	}

public:
	eastl::shared_ptr<T> Data;
};

struct SelfRegisteringUniform
{
	SelfRegisteringUniform();
	SelfRegisteringUniform(const int32_t inValue);
	SelfRegisteringUniform(const uint32_t inValue);
	SelfRegisteringUniform(const float inValue);
	SelfRegisteringUniform(const glm::mat4& inValue);
	SelfRegisteringUniform(const glm::vec2& inValue);
	SelfRegisteringUniform(const glm::vec3& inValue);
	SelfRegisteringUniform(const glm::vec4& inValue);
	SelfRegisteringUniform(const eastl::vector<float>& inValue);
	SelfRegisteringUniform(const eastl::vector<glm::vec4>& inValue);
	SelfRegisteringUniform(const eastl::vector<glm::mat4>& inValue);
	SelfRegisteringUniform(const eastl::vector<SPointLight>& inValue);
	SelfRegisteringUniform(const eastl::shared_ptr<RHITexture2D>& inValue);

	void Register(class UniformBufferContainer& inBuffer, const size_t inRequiredCount) const;

public:

	template<typename T>
	T& GetValue() 
	{
		SelfRegisteringUniformData<T>* typedUniform = static_cast<SelfRegisteringUniformData<T>*>(Data.get());
		return typedUniform->Data;
	}

	eastl::unique_ptr<IUniformData> Data;
};