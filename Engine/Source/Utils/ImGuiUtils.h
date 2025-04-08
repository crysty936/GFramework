#pragma once

namespace ImGuiUtils
{
	struct ImGuiScope
	{
		ImGuiScope(const char* InName);
		~ImGuiScope();
	};
}