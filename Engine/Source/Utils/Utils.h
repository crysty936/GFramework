#pragma once
#include <stdint.h>
#include "Core/EngineUtils.h"
#include "EASTL/string.h"

namespace Utils
{
	inline uint64_t AlignTo(const uint64_t inNum, uint64_t inAlignment)
	{
		ASSERT(inAlignment > 0);

		return ((inNum + inAlignment - 1) / inAlignment) * inAlignment;
	}

	eastl::string GetTimeString();

}
