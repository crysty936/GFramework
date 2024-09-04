#include "Utils/IOUtils.h"
#include <fstream>
#include <filesystem>
#include "Logger/Logger.h"
#include "Core/EngineUtils.h"

namespace IOUtils
{
	bool TryFastReadFile(const eastl::string& inFilePath, eastl::string& outData)
	{
		std::ifstream fileStream(inFilePath.data());

		if (!fileStream.is_open())
		{
			LOG_ERROR("Failed to open file %s.", inFilePath.data());

			ASSERT(false);

			return false;
		}

		const uintmax_t size = std::filesystem::file_size(inFilePath.data());
		outData.resize(size + 1, ' ');

		fileStream.read(outData.data(), size);
		fileStream.close();

		return true;
	}
}
