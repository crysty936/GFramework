#pragma once
#include "EASTL/string.h"

namespace PathUtils
{

	inline void ReplaceChar(wchar_t* str, wchar_t find, wchar_t replace) {
		wchar_t* current_pos = wcschr(str, find);
		while (current_pos) {
			*current_pos = replace;
			current_pos = wcschr(current_pos, find);
		}
	}

	inline void ReplaceChar(char* str, char find, char replace) {
		char* current_pos = strchr(str, find);
		while (current_pos) {
			*current_pos = replace;
			current_pos = strchr(current_pos, find);
		}
	}

	inline void NormalizeDirectory(eastl::string& inOutPath)
	{
		ReplaceChar(const_cast<char*>(inOutPath.c_str()), '\\', '/');
	}

	inline void NormalizeDirectory(eastl::wstring& inOutPath)
	{
		ReplaceChar(const_cast<wchar_t*>(inOutPath.c_str()), '\\', '/');
	}
}
