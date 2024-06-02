#include "ImageLoading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Core/EngineUtils.h"
#include <mutex>

// TODO: Handle multiple threads with stbi
// At the moment, global flip vertically flag is making multithreading on this impossible
static std::mutex stbiMutex;

ImageData ImageLoading::LoadImageData(const char* inTexurePath, const bool inFlipped, const int32_t inNrChannels)
{
	std::lock_guard<std::mutex> lock(stbiMutex);
	stbi_set_flip_vertically_on_load(inFlipped);

	unsigned char* rawData = nullptr;
	int32_t width, height, nrChannels;
	width, height, nrChannels = 0;

	rawData = stbi_load(inTexurePath, &width, &height, &nrChannels, 4);

	if (!ENSURE_MSG(rawData, "Image %s Loading Failed with reason: %s", inTexurePath, stbi__g_failure_reason))
	{
		// retry
		rawData = stbi_load(inTexurePath, &width, &height, &nrChannels, 4);

		return {};
	}

	ImageData data{ rawData, 4, width, height };

	return data;
}

void ImageLoading::FreeImageData(ImageData inData)
{
	stbi_image_free(inData.RawData);
}
