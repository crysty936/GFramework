#pragma once
#include <stdint.h>
#include "EASTL/string.h"
#include "EASTL/unique_ptr.h"

enum class ETextureType
{
	Single, // 2D
	Array
};

enum class ERHITextureChannelsType
{
	RGBA,
	Depth,
	DepthStencil
};

enum class ERHITexturePrecision
{
	UnsignedByte,
	Float16,
	Float32,
};

enum class ERHITextureFilter
{
	Linear,
	Nearest
};

enum class ETextureState
{
	Present,
	Shader_Resource,
	Render_Target,
	Depth_Write
};

class RHITexture2D
{
public:
	int32_t NrChannels = 0;
	uint64_t Width = 0;
	uint64_t Height = 0;
	uint16_t NumMips = 0;

	// TODO: Debug only
	eastl::string SourcePath;

	ETextureType TextureType = ETextureType::Single;
	ERHITextureChannelsType ChannelsType = ERHITextureChannelsType::RGBA;
	ERHITexturePrecision Precision = ERHITexturePrecision::UnsignedByte;
	ERHITextureFilter Filter = ERHITextureFilter::Linear;
};

