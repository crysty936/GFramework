#pragma once

#include <stdint.h>
#include "Core/EngineUtils.h"
#include "EASTL/string.h"

enum class VertexPropertyType : uint32_t
{
	UInt,
	Float
};

enum class VertexInputType : uint32_t
{
	Undefined = 0,
	Position,
	Normal,
	TexCoords,
	Tangent,
	Bitangent,
	InstanceData,
	Count
};

enum class EAttribDivisor : uint32_t
{
	PerVertex = 0,
	PerInstance = 1
};

struct VertexLayoutProperties
{
	/** Float or UInt */
	VertexPropertyType Type;
	/** 3 for Basic Vector */
	uint32_t Count;

	uint32_t bNormalized;

	VertexInputType InputType = VertexInputType::Undefined;

	uint32_t GetSizeOfType() const;

	EAttribDivisor Divisor = EAttribDivisor::PerVertex;
};


