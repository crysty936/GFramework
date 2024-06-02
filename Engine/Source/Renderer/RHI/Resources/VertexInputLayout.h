#pragma once

#include "EASTL/vector.h"
#include "VertexLayoutProperties.h"

class VertexInputLayout
{

public:
	VertexInputLayout()
		:Stride(0) {}


	template<typename T>
	void Push(uint32_t Count, const VertexInputType inType, const EAttribDivisor inDivisor = EAttribDivisor::PerVertex)
	{
		static_assert(false);
	}

	template<>
	void Push<float>(uint32_t Count, const VertexInputType inType, const EAttribDivisor inDivisor)
	{
		Properties.push_back({ VertexPropertyType::Float, Count, false, inType, inDivisor });
		Stride += Count * sizeof(float);
	}

	template<>
	void Push<uint32_t>(uint32_t Count, const VertexInputType inType, const EAttribDivisor inDivisor)
	{
		Properties.push_back({ VertexPropertyType::UInt, Count, false, inType,  inDivisor });
		Stride += Count * sizeof(uint32_t);
	}

	inline const eastl::vector<VertexLayoutProperties>& GetProperties() const { return Properties; }
	inline const uint32_t GetStride() const { return Stride; }

public:
	eastl::vector<VertexLayoutProperties> Properties;
	uint32_t Stride;
	// For having attributes depending on multiple buffers, any buffer that's not the first will need to continue the attribs index 
	int32_t AttribsOffset = 0;
};
