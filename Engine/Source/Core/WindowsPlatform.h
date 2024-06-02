#pragma once
#include <stdint.h>
#include "InputSystem/InputKeys.h"
#include "InputSystem/CursorMode.h"
#include "InputSystem/InputType.h"
#include "EASTL/string.h"


struct HKEY__;

enum class CLITextColor
{
	Red,
	Yellow,
	White
};

namespace Windows
{
	typedef HKEY__* HKEY;
}

// Wrapper for windows.h to not pollute the dev environment
namespace WindowsPlatform
{


	void InitCycles();
	double GetTime();
	void Sleep(uint32_t inMilliseconds);
	void SetCLITextColor(CLITextColor inColor);
	EInputKey WindowsKeyToInternal(const int16_t inWindowsKey);
	void PoolMessages();
	void* CreateWindowsWindow(const int32_t desiredWidth, const int32_t desiredHeight);
	void SetCursorMode(void* inWindowHandle, const ECursorMode inMode);
	void SetWindowsWindowText(const eastl::wstring& inText);
	void DrawImGUIAdditionalWindows();

	void ForwardKeyInput(const EInputKey inKey, const EInputType inAction);
	void ForwardMouseMoveInput(double inNewYaw, double inNewPitch);

	bool QueryRegKey(const Windows::HKEY InKey, const wchar_t* InSubKey, const wchar_t* InValueName, eastl::wstring& OutData);
	bool DirectoryExistsInternal(const eastl::string& inPath);
	bool CreateDirectoryTree(const eastl::string& Directory);
	bool CreateDirectoryInternal(const eastl::string& Directory);
}
