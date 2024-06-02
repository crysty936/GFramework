#pragma once
#include "EASTL/string_view.h"

struct WindowProperties
{
	eastl::string_view Title = "MainWindow";
	uint32_t Width = 1920;
	uint32_t Height = 1080;
	float AspectRatio = 1920.f / 1080.f;
	bool VSyncEnabled = false;
};