#include "VertexLayoutProperties.h"

uint32_t VertexLayoutProperties::GetSizeOfType() const
{
	switch (Type)
	{
	case VertexPropertyType::UInt:
		return sizeof(uint32_t);
	case VertexPropertyType::Float:
		return sizeof(float);
	default:
		return 0;
	}
}

