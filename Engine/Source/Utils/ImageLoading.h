#pragma once

#include <stdint.h>
#include "Core/EngineUtils.h"

/**
 * Wrapper for stb_image
 */

struct ImageData
{
	void* RawData = nullptr;
	int32_t NrChannels = 0;
	int32_t Width = 0;
	int32_t Height = 0;
};

namespace ImageLoading
{
	ImageData LoadImageData(const char* inTexurePath, const bool inFlipped = true, const int32_t inNrChannels = 4);
	void FreeImageData(ImageData inData);
}

