#pragma once
#include "EASTL/string.h"

namespace IOUtils
{
	bool TryFastReadFile(const eastl::string& inFilePath, eastl::string& outData);
}
