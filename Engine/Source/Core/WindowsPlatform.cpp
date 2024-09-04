
#ifndef UNICODE
#define UNICODE
#endif

#include "Core/WindowsPlatform.h"
#include <windows.h>
#include <WinUser.h>
#include <winnls.h>
#include "Logger/Logger.h"
#include "InputSystem/InputType.h"
#include "InputSystem/InputSystem.h"
#include "Window/WindowsWindow.h"
#include <atlbase.h>
#include <atlwin.h>
#include "EASTL/string.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "imgui_internal.h"
#include "Utils/PathUtils.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Wrapper over Windows.h functionality
namespace WindowsPlatform
{
	 // Timing

	static double SecondsPerCycle = 0;

	eastl::string GetWin32ErrorStringAnsi(uint32_t inErrorCode)
	{
		char errorString[MAX_PATH];
		::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
			0,
			inErrorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorString,
			MAX_PATH,
			NULL);

		eastl::string message = "Win32 Error: ";
		message += errorString;
		return message;
	}

	eastl::wstring GetWin32ErrorString(uint32_t inErrorCode)
	{
		wchar_t errorString[MAX_PATH];
		::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			0,
			inErrorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorString,
			MAX_PATH,
			NULL);

		eastl::wstring message = L"Win32 Error: ";
		message += errorString;
		return message;
	}

	eastl::string GetExePathAnsi() {
		char buffer[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		eastl::string stringBuffer = eastl::string(buffer);
		eastl::string::size_type pos = stringBuffer.find_last_of("\\");
		return eastl::string(buffer).substr(0, pos);
	}

	eastl::wstring GetExePath() {
		wchar_t buffer[MAX_PATH] = { 0 };
		GetModuleFileNameW(NULL, buffer, MAX_PATH);
		eastl::wstring stringBuffer = eastl::wstring(buffer);
		eastl::wstring::size_type pos = stringBuffer.find_last_of(L"\\");
		return eastl::wstring(buffer).substr(0, pos);
	}

	void InitCycles()
	{
		// Set the main thread to work only on one core to avoid timing synchrionization problems between multiple cores
		// Source: https://docs.microsoft.com/en-us/windows/win32/dxtecharts/game-timing-and-multicore-processors
		DWORD_PTR threadMask = static_cast<DWORD_PTR>(1);
		::SetThreadAffinityMask(::GetCurrentThread(), threadMask);

		// Initialise required frequency for seconds calc
		::LARGE_INTEGER Frequency;
		::QueryPerformanceFrequency(&Frequency);
		SecondsPerCycle = 1.0 / Frequency.QuadPart;
	}

	double GetTime()
	{
		// Making use of Windows Platform functions for getting the time
		::LARGE_INTEGER cycles;
		::QueryPerformanceCounter(&cycles);

		return cycles.QuadPart * SecondsPerCycle + 16777216.0;
	}

 	void Sleep(uint32_t inMilliseconds)
 	{
 		::Sleep(inMilliseconds);
 	}

	// CLI

	void SetCLITextColor(CLITextColor inColor)
	{
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		WORD color = 0;

		switch (inColor)
		{
		case CLITextColor::Red:
		{
			color = FOREGROUND_RED;
			break;
		}
		case CLITextColor::Yellow:
		{
			color = FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		}
		case CLITextColor::White:
		{
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
		}
		}

		SetConsoleTextAttribute(hStdOut, color);
	}


	// Input
	EInputType KeyStates[+EInputKey::Count];
	bool bReactToMouseInput = true;
    bool bRawMouseInput = false;
	RAWINPUT* rawInput;

	class WindowsKeysContainer
	{
	public:
		WindowsKeysContainer()
		{
			Codes[0x00B] = +EInputKey::D0;
			Codes[0x002] = +EInputKey::D1;
			Codes[0x003] = +EInputKey::D2;
			Codes[0x004] = +EInputKey::D3;
			Codes[0x005] = +EInputKey::D4;
			Codes[0x006] = +EInputKey::D5;
			Codes[0x007] = +EInputKey::D6;
			Codes[0x008] = +EInputKey::D7;
			Codes[0x009] = +EInputKey::D8;
			Codes[0x00A] = +EInputKey::D9;
			Codes[0x01E] = +EInputKey::A;
			Codes[0x030] = +EInputKey::B;
			Codes[0x02E] = +EInputKey::C;
			Codes[0x020] = +EInputKey::D;
			Codes[0x012] = +EInputKey::E;
			Codes[0x021] = +EInputKey::F;
			Codes[0x022] = +EInputKey::G;
			Codes[0x023] = +EInputKey::H;
			Codes[0x017] = +EInputKey::I;
			Codes[0x024] = +EInputKey::J;
			Codes[0x025] = +EInputKey::K;
			Codes[0x026] = +EInputKey::L;
			Codes[0x032] = +EInputKey::M;
			Codes[0x031] = +EInputKey::N;
			Codes[0x018] = +EInputKey::O;
			Codes[0x019] = +EInputKey::P;
			Codes[0x010] = +EInputKey::Q;
			Codes[0x013] = +EInputKey::R;
			Codes[0x01F] = +EInputKey::S;
			Codes[0x014] = +EInputKey::T;
			Codes[0x016] = +EInputKey::U;
			Codes[0x02F] = +EInputKey::V;
			Codes[0x011] = +EInputKey::W;
			Codes[0x02D] = +EInputKey::X;
			Codes[0x015] = +EInputKey::Y;
			Codes[0x02C] = +EInputKey::Z;

			Codes[0x028] = +EInputKey::Apostrophe;
			Codes[0x02B] = +EInputKey::Backslash;
			Codes[0x033] = +EInputKey::Comma;
			Codes[0x00D] = +EInputKey::Equal;
			Codes[0x029] = +EInputKey::GraveAccent;
			Codes[0x01A] = +EInputKey::LeftBracket;
			Codes[0x00C] = +EInputKey::Minus;
			Codes[0x034] = +EInputKey::Period;
			Codes[0x01B] = +EInputKey::RightBracket;
			Codes[0x027] = +EInputKey::Semicolon;
			Codes[0x035] = +EInputKey::Slash;
			Codes[0x056] = +EInputKey::World2;

			Codes[0x00E] = +EInputKey::Backspace;
			Codes[0x153] = +EInputKey::Delete;
			Codes[0x14F] = +EInputKey::End;
			Codes[0x01C] = +EInputKey::Enter;
			Codes[0x001] = +EInputKey::Escape;
			Codes[0x147] = +EInputKey::Home;
			Codes[0x152] = +EInputKey::Insert;
			Codes[0x15D] = +EInputKey::Menu;
			Codes[0x151] = +EInputKey::PageDown;
			Codes[0x149] = +EInputKey::PageUp;
			Codes[0x045] = +EInputKey::Pause;
			Codes[0x146] = +EInputKey::Pause;
			Codes[0x039] = +EInputKey::Space;
			Codes[0x00F] = +EInputKey::Tab;
			Codes[0x03A] = +EInputKey::CapsLock;
			Codes[0x145] = +EInputKey::NumLock;
			Codes[0x046] = +EInputKey::ScrollLock;
			Codes[0x03B] = +EInputKey::F1;
			Codes[0x03C] = +EInputKey::F2;
			Codes[0x03D] = +EInputKey::F3;
			Codes[0x03E] = +EInputKey::F4;
			Codes[0x03F] = +EInputKey::F5;
			Codes[0x040] = +EInputKey::F6;
			Codes[0x041] = +EInputKey::F7;
			Codes[0x042] = +EInputKey::F8;
			Codes[0x043] = +EInputKey::F9;
			Codes[0x044] = +EInputKey::F10;
			Codes[0x057] = +EInputKey::F11;
			Codes[0x058] = +EInputKey::F12;
			Codes[0x064] = +EInputKey::F13;
			Codes[0x065] = +EInputKey::F14;
			Codes[0x066] = +EInputKey::F15;
			Codes[0x067] = +EInputKey::F16;
			Codes[0x068] = +EInputKey::F17;
			Codes[0x069] = +EInputKey::F18;
			Codes[0x06A] = +EInputKey::F19;
			Codes[0x06B] = +EInputKey::F20;
			Codes[0x06C] = +EInputKey::F21;
			Codes[0x06D] = +EInputKey::F22;
			Codes[0x06E] = +EInputKey::F23;
			Codes[0x076] = +EInputKey::F24;
			Codes[0x038] = +EInputKey::LeftAlt;
			Codes[0x01D] = +EInputKey::LeftControl;
			Codes[0x02A] = +EInputKey::LeftShift;
			Codes[0x15B] = +EInputKey::LeftSuper;
			Codes[0x137] = +EInputKey::PrintScreen;
			Codes[0x138] = +EInputKey::RightAlt;
			Codes[0x11D] = +EInputKey::RightControl;
			Codes[0x036] = +EInputKey::RightShift;
			Codes[0x15C] = +EInputKey::RightSuper;
			Codes[0x150] = +EInputKey::Down;
			Codes[0x14B] = +EInputKey::Left;
			Codes[0x14D] = +EInputKey::Right;
			Codes[0x148] = +EInputKey::Up;

			Codes[0x052] = +EInputKey::KP0;
			Codes[0x04F] = +EInputKey::KP1;
			Codes[0x050] = +EInputKey::KP2;
			Codes[0x051] = +EInputKey::KP3;
			Codes[0x04B] = +EInputKey::KP4;
			Codes[0x04C] = +EInputKey::KP5;
			Codes[0x04D] = +EInputKey::KP6;
			Codes[0x047] = +EInputKey::KP7;
			Codes[0x048] = +EInputKey::KP8;
			Codes[0x049] = +EInputKey::KP9;
			Codes[0x04E] = +EInputKey::KPAdd;
			Codes[0x053] = +EInputKey::KPDecimal;
			Codes[0x135] = +EInputKey::KPDivide;
			Codes[0x11C] = +EInputKey::KPEnter;
			Codes[0x059] = +EInputKey::KPEqual;
			Codes[0x037] = +EInputKey::KPMultiply;
			Codes[0x04A] = +EInputKey::KPSubtract;
		}

		int16_t Codes[512];

		int16_t GetCode(const int16_t inCode)
		{
			return Codes[inCode];
		}
	};

	WindowsKeysContainer KeysContainer;

	EInputKey WindowsKeyToInternal(const int16_t inWindowsKey)
	{
		return static_cast<EInputKey>(KeysContainer.GetCode(inWindowsKey));
	}


	void UpdateClipRect(HWND inWindowHandle)
	{
        LOG_INFO("Cursor Clip Rect updated.")

		if (inWindowHandle)
		{
			RECT clipRect;
			GetClientRect(inWindowHandle, &clipRect);
			ClientToScreen(inWindowHandle, (POINT*)&clipRect.left);
			ClientToScreen(inWindowHandle, (POINT*)&clipRect.right);
			ClipCursor(&clipRect);
		}
		else
		{
			ClipCursor(nullptr);
		}
	}

	bool IsWindowFocused(HWND inWindowHandle)
	{
		return inWindowHandle == GetActiveWindow();
	}

	void GetCursorPos(HWND inWindowHandle, OUT glm::vec<2, int>& outPos)
	{
		POINT pos;

		if (GetCursorPos(&pos))
		{
			ScreenToClient(inWindowHandle, &pos);

			outPos.x = pos.x;
			outPos.y = pos.y;
		}
	}

	void InternalSetCursorPos(HWND inWindowHandle, const glm::vec<2, int>& inPos)
	{
		POINT pos = { inPos.x, inPos.y };

		// Store the new position so it can be recognized later
		InputSystem::Get().LastMousePos.x = pos.x;
		InputSystem::Get().LastMousePos.y = pos.y;

		ClientToScreen(inWindowHandle, &pos);
		::SetCursorPos(pos.x, pos.y);
	}

	void DisableCursor(HWND inWindowHandle)
	{
		GetCursorPos(inWindowHandle, InputSystem::Get().CurrentCursorPos);
		SetCursor(nullptr);

		RECT area;
		GetClientRect(inWindowHandle, &area);
		const int width = area.right;
		const int height = area.bottom;

        InternalSetCursorPos(inWindowHandle, { width / 2, height / 2 });

		UpdateClipRect(inWindowHandle);

		bReactToMouseInput = true;

        // enable raw mouse motion
		const RAWINPUTDEVICE rid = { 0x01, 0x02, 0, inWindowHandle };
        ENSURE_MSG(RegisterRawInputDevices(&rid, 1, sizeof(rid)), "Win32: Failed to register raw input device.");
		bRawMouseInput = true;

		LOG_INFO("Disabled cursor.");
	}

	void EnableCursor(HWND inWindowHandle)
	{
		// disable raw mouse motion
		const RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, nullptr };
		ENSURE_MSG(RegisterRawInputDevices(&rid, 1, sizeof(rid)), "Win32: Failed to remove raw input device.");
		bRawMouseInput = false;

		UpdateClipRect(nullptr);

		InternalSetCursorPos(inWindowHandle, InputSystem::Get().CurrentCursorPos);

        // Enable mouse icon
		SetCursor(LoadCursorW(nullptr, IDC_ARROW));
		InputSystem::Get().CursorsTracked = false;

		bReactToMouseInput = false;

		LOG_INFO("Enabled cursor.");
	}

	void SetCursorMode(void* inWindowHandle, const ECursorMode inMode)
	{
		ImGuiIO& io = ImGui::GetIO();

		switch (inMode)
        {
        case ECursorMode::Enabled:
        {
			EnableCursor(static_cast<HWND>(inWindowHandle));
            io.EnableCursor();

            break;
        }
        case ECursorMode::Hidden:
        {

            break;
        }
        case ECursorMode::Disabled:
        {
			if (IsWindowFocused(static_cast<HWND>(inWindowHandle)))
			{
				DisableCursor(static_cast<HWND>(inWindowHandle));
                io.DisableCursor();
			}
            break;
        }
        }
	}

    void UpdateCursorImage(HWND inWindowHandle)
    {
        if (InputSystem::Get().CurrentCursorMode == ECursorMode::Enabled)
        {
			SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
        else
        {
            SetCursor(nullptr);
        }
    }

	void SetWindowsWindowText(const eastl::wstring& inText)
	{
		SetWindowTextW(static_cast<HWND>(GEngine->GetMainWindow().GetHandle()), inText.c_str());
	}

	void ForwardKeyInput(const EInputKey inKey, const EInputType inAction)
	{
		InputSystem::KeyCallback(inKey, inAction);
	}

	void ForwardMouseMoveInput(double inNewYaw, double inNewPitch)
	{
		InputSystem::Get().VirtualMousePos.x = inNewYaw;
		InputSystem::Get().VirtualMousePos.y = inNewPitch;

		InputSystem::MousePosChangedCallback(inNewYaw, inNewPitch);
	}

	void ForwardMouseScrollInput(float inWheelDelta, const glm::vec<2, int> inCursorPos)
	{

		InputSystem::MouseScrollCallback(0, inWheelDelta);
	}

	bool QueryRegKey(const Windows::HKEY InKey, const wchar_t* InSubKey, const wchar_t* InValueName, eastl::wstring& OutData)
	{
		bool bSuccess = false;

		// Redirect key depending on system
		for (int32_t RegistryIndex = 0; RegistryIndex < 2 && !bSuccess; ++RegistryIndex)
		{
			HKEY Key = 0;
			const uint32_t RegFlags = (RegistryIndex == 0) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
			if (RegOpenKeyEx(InKey, InSubKey, 0, KEY_READ | RegFlags, &Key) == ERROR_SUCCESS)
			{
				::DWORD Size = 0;
				// First, we'll call RegQueryValueEx to find out how large of a buffer we need
				if ((RegQueryValueEx(Key, InValueName, NULL, NULL, NULL, &Size) == ERROR_SUCCESS) && Size)
				{
					// Allocate a buffer to hold the value and call the function again to get the data
                    wchar_t* Buffer = new wchar_t[Size];
					if (RegQueryValueEx(Key, InValueName, NULL, NULL, (LPBYTE)Buffer, &Size) == ERROR_SUCCESS)
					{
						const uint32_t Length = (Size / sizeof(wchar_t)) - 1;
						OutData = eastl::wstring((wchar_t*)Buffer, Length);
						bSuccess = true;
					}
					delete[] Buffer;
				}
				RegCloseKey(Key);
			}
		}

		return bSuccess;
	}

	eastl::string WindowsNormalizedPath(const eastl::string& PathString)
	{
		eastl::string outputString = eastl::string(PathString);
		PathUtils::ReplaceChar(const_cast<char*>(outputString.c_str()), '/', '\\');

		//size_t length = outputString.length();
		//for (size_t i = 0; i < length; ++i)
		//{
		//	char* currentChar = &outputString[i];
		//	if (*currentChar == '\\')
		//	{
		//		outputString.insert(i, eastl::string("\\"));
		//		++i;
		//	}
		//}

		return outputString;

		//FString Result = FPaths::ConvertRelativePathToFull(FString(PathString));

		//// NormalizeFilename was already called by ConvertRelativePathToFull, but we still need to do the extra steps in NormalizeDirectoryName if it is a directory
		//FPaths::NormalizeDirectoryName(Result);

		//// Remove duplicate slashes
		//const bool bIsUNCPath = Result.StartsWith(TEXT("//"));
		//if (bIsUNCPath)
		//{
		//	// Keep // at the beginning.  If There are more than two / at the beginning, replace them with just //.
		//	FPaths::RemoveDuplicateSlashes(Result);
		//	Result = TEXT("/") + Result;
		//}
		//else
		//{
		//	FPaths::RemoveDuplicateSlashes(Result);
		//}

		//// We now have a canonical, strict-valid, absolute Unreal Path.  Convert it to a Windows Path.
		//Result.ReplaceCharInline(TEXT('/'), TEXT('\\'), ESearchCase::CaseSensitive);

		//// Handle Windows Path length over MAX_PATH
		//if (Result.Len() > MAX_PATH)
		//{
		//	if (bIsUNCPath)
		//	{
		//		Result = TEXT("\\\\?\\UNC") + Result.RightChop(1);
		//	}
		//	else
		//	{
		//		Result = TEXT("\\\\?\\") + Result;
		//	}
		//}

		//return Result;
	}


	bool DirectoryExistsInternal(const eastl::string& inPath)
	{
		eastl::string normalizedPath = WindowsNormalizedPath(inPath);
		uint32_t Result = GetFileAttributesA(normalizedPath.c_str());
		const bool bExists = (Result != 0xFFFFFFFF && (Result & FILE_ATTRIBUTE_DIRECTORY));

        return bExists;
	}

	bool CreateDirectoryTree(const eastl::string& Directory)
	{
		if (CreateDirectoryInternal(Directory))
			return true;

		const size_t separatorIndex = Directory.find_last_of("\\");
		if (separatorIndex != eastl::string::npos)
		{
			if (!CreateDirectoryTree(Directory.left(separatorIndex)))
			{
				return false;
			}

			if (CreateDirectoryInternal(Directory))
			{
				return true;
			}
		}

		return DirectoryExistsInternal(Directory);
	}

	bool CreateDirectoryInternal(const eastl::string& Directory)
	{
		eastl::string normalizedPath = WindowsNormalizedPath(Directory);
		bool success = ::CreateDirectoryA(normalizedPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;;

		return success;
	}


	// Message Loop

	void PoolMessages()
	{
		MSG msg = {};

		// If there are Window messages, process them
		while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}


#if 0
#define LOG_WINMSG(MsgType) LOG_INFO("Windows Message: %s", #MsgType)
#else
#define LOG_WINMSG(MsgType) 
#endif

	LRESULT WindowMessageLoop(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
	{

		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

        // Don't treat input if mouse is over ImGui window
		if (GImGui)
		{
			if (GImGui->HoveredWindow)
			{
                return true;
			}
		}

		// Forward hwnd on because we can get messages (e.g., WM_CREATE)
		// before CreateWindow returns, and thus before mhMainWnd is valid.

		// This is set as 1 after the window's creation
		void* windowValid = GetPropW(hwnd, L"WindowProp");

		if (!windowValid)
		{
			// This is the message handling for the dummy window
			// and for a regular window during its initial creation

			return DefWindowProcW(hwnd, msg, wParam, lParam);
		}

		 //Whole glfw message loop but use only what's needed
		switch (msg)
		{
 		case WM_MOUSEACTIVATE:
 		{
 			LOG_WINMSG(WM_MOUSEACTIVATE);
 
 			// HACK: Postpone cursor disabling when the window was activated by
 			// clicking a caption button
 			if (HIWORD(lParam) == WM_LBUTTONDOWN)
 			{
 				if (LOWORD(lParam) != HTCLIENT)
 				{
 					InputSystem::Get().OngoingFrameAction = true;
 				}
 			}
 
 			break;
 		}
 
 		case WM_CAPTURECHANGED:
 		{
 			LOG_WINMSG(WM_CAPTURECHANGED);
 			// HACK: Disable the cursor once the caption button action has been
 			// completed or cancelled
 			if (lParam == 0 && InputSystem::Get().OngoingFrameAction)
 			{
 				if (InputSystem::Get().CurrentCursorMode == ECursorMode::Disabled)
 				{
 					DisableCursor(hwnd);
 				}
 
 				InputSystem::Get().OngoingFrameAction = false;
 			}
 
 			break;
 		}
 
 		case WM_SETFOCUS:
 		{
 			LOG_WINMSG(WM_SETFOCUS);
 			// _glfwInputWindowFocus(window, GLFW_TRUE);
 			// HACK: Do not disable cursor while the user is interacting with
 			//       a caption button
 			if (InputSystem::Get().OngoingFrameAction)
 				break;
 
 			if (InputSystem::Get().CurrentCursorMode == ECursorMode::Disabled)
 			{
 				DisableCursor(hwnd);
 			}
 
 
 			return 0;
 		}
 
 		case WM_KILLFOCUS:
 		{
 			LOG_WINMSG(WM_KILLFOCUS);
 			if (InputSystem::Get().CurrentCursorMode == ECursorMode::Disabled)
 			{
 				EnableCursor(hwnd);
 			}
 			// 
 			//             if (window->monitor && window->autoIconify)
 			//                 _glfwPlatformIconifyWindow(window);
 			// 
 			//             _glfwInputWindowFocus(window, GLFW_FALSE);
 
 			return 0;
 		}
 
 		case WM_SYSCOMMAND:
 		{
 			LOG_WINMSG(WM_SYSCOMMAND);
 			switch (wParam & 0xfff0)
 			{
 			case SC_SCREENSAVE:
 			case SC_MONITORPOWER:
 			{
 				LOG_WINMSG(SC_SCREENSAVE - SC_MONITORPOWER);
 //				if (window->monitor)
 //				{
 //					// We are running in full screen mode, so disallow
 //					// screen saver and screen blanking
 //					return 0;
 //              }
 				break;
 			}
 
 			// User trying to access application menu using ALT?
 			case SC_KEYMENU:
 			{
 				LOG_WINMSG(SC_KEYMENU);
 
 				return 0;
 			}
 			}
 			break;
 		}
 
 		case WM_CLOSE:
 		{
 			LOG_WINMSG(WM_CLOSE);
            GEngine->GetMainWindow().RequestClose();
 
 			return 0;
 		}
 
 		case WM_INPUTLANGCHANGE:
 		{
 			LOG_WINMSG(WM_INPUTLANGCHANGE);
 			//_glfwUpdateKeyNamesWin32();
 			break;
 		}
 
 		case WM_CHAR:
 		case WM_SYSCHAR:
 		{
 			LOG_WINMSG(WM_CHAR - WM_SYSCHAR);
 			// if (wParam >= 0xd800 && wParam <= 0xdbff)
 //                 window->win32.highSurrogate = (WCHAR) wParam;
 //             else
 //             {
 //                 unsigned int codepoint = 0;
 // 
 //                 if (wParam >= 0xdc00 && wParam <= 0xdfff)
 //                 {
 //                     if (window->win32.highSurrogate)
 //                     {
 //                         codepoint += (window->win32.highSurrogate - 0xd800) << 10;
 //                         codepoint += (WCHAR) wParam - 0xdc00;
 //                         codepoint += 0x10000;
 //                     }
 //                 }
 //                 else
 //                     codepoint = (WCHAR) wParam;
 // 
 //                 window->win32.highSurrogate = 0;
 //                 _glfwInputChar(window, codepoint, getKeyMods(), uMsg != WM_SYSCHAR);
 //             }
 
 			return 0;
 		}
 
 		case WM_UNICHAR:
 		{
 			LOG_WINMSG(WM_UNICHAR);
 			if (wParam == UNICODE_NOCHAR)
 			{
 				// WM_UNICHAR is not sent by Windows, but is sent by some
 				// third-party input method engine
 				// Returning TRUE here announces support for this message
 				return TRUE;
 			}
 
 			//_glfwInputChar(window, (unsigned int) wParam, getKeyMods(), GLFW_TRUE);
 			return 0;
 		}
 
  		case WM_KEYDOWN:
  		case WM_SYSKEYDOWN:
  		case WM_KEYUP:
  		case WM_SYSKEYUP:
  		{
  			LOG_WINMSG(WM_KEYDOWN - WM_SYSKEYDOWN - WM_KEYUP - WM_SYSKEYUP);
  			int32_t scancode = 0;
  			EInputKey key = EInputKey::None;
  			EInputType action = (HIWORD(lParam) & KF_UP) ? EInputType::InputRelease : EInputType::InputPress;
  
  			//             const int mods = getKeyMods();
  			// 
  			scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
  			if (!scancode)
  			{
  				// NOTE: Some synthetic key messages have a scancode of zero
  				// HACK: Map the virtual key back to a usable scancode
  				scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
  			}
  
  			key = WindowsPlatform::WindowsKeyToInternal(scancode);
  
  			if (KeyStates[+key] == EInputType::InputPress && action == EInputType::InputPress)
  			{
  				action = EInputType::InputRepeat;
  			}
  			else
  			{
  				KeyStates[+key] = action;
  			}
  
  
  			// The Ctrl keys require special handling
  			if (wParam == VK_CONTROL)
  			{
  				if (HIWORD(lParam) & KF_EXTENDED)
  				{
  					// Right side keys have the extended key bit set
  					key = EInputKey::RightControl;
  				}
  				else
  				{
  					// NOTE: Alt Gr sends Left Ctrl followed by Right Alt
  					// HACK: We only want one event for Alt Gr, so if we detect
  					//       this sequence we discard this Left Ctrl message now
  					//       and later report Right Alt normally
  					MSG next;
  					const DWORD time = GetMessageTime();
  
  					if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE))
  					{
  						if (next.message == WM_KEYDOWN ||
  							next.message == WM_SYSKEYDOWN ||
  							next.message == WM_KEYUP ||
  							next.message == WM_SYSKEYUP)
  						{
  							if (next.wParam == VK_MENU &&
  								(HIWORD(next.lParam) & KF_EXTENDED) &&
  								next.time == time)
  							{
  								// Next message is Right Alt down so discard this
  								break;
  							}
  						}
  					}
  
  					// This is a regular Left Ctrl message
  					key = EInputKey::LeftControl;
  				}
  			}
  			else if (wParam == VK_PROCESSKEY)
  			{
  				// IME notifies that keys have been filtered by setting the
  				// virtual key-code to VK_PROCESSKEY
  				break;
  			}
  
  			if (action == EInputType::InputRelease && wParam == VK_SHIFT)
  			{
  				// HACK: Release both Shift keys on Shift up event, as when both
  				//       are pressed the first release does not emit any event
  				// NOTE: The other half of this is in _glfwPlatformPollEvents
  				//_glfwInputKey(window, GLFW_KEY_LEFT_SHIFT, scancode, action, mods);
  				//_glfwInputKey(window, GLFW_KEY_RIGHT_SHIFT, scancode, action, mods);
  			}
  			else if (wParam == VK_SNAPSHOT)
  			{
  				// HACK: Key down is not reported for the Print Screen key
  				//_glfwInputKey(window, key, scancode, GLFW_PRESS, mods);
  				//_glfwInputKey(window, key, scancode, GLFW_RELEASE, mods);
  			}
  			else
  			{
                ForwardKeyInput(key, action);
			}
  
  			break;
  		}
 
 		case WM_LBUTTONDOWN:
 		case WM_RBUTTONDOWN:
 		case WM_MBUTTONDOWN:
 		case WM_XBUTTONDOWN:
 		case WM_LBUTTONUP:
 		case WM_RBUTTONUP:
 		case WM_MBUTTONUP:
 		case WM_XBUTTONUP:
 		{
 			LOG_WINMSG(WM_LBUTTONDOWN - WM_RBUTTONDOWN - WM_MBUTTONDOWN - WM_XBUTTONDOWN - WM_LBUTTONUP - WM_RBUTTONUP - WM_MBUTTONUP - WM_XBUTTONUP);
            EInputKey inputKey;
            EInputType inputType;
 			//int i, button, action;
  
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
                inputKey = EInputKey::MouseLeft;
            else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)
                inputKey = EInputKey::MouseRight;
//             else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP)
//                 button = GLFW_MOUSE_BUTTON_MIDDLE;
//             else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
//                 button = GLFW_MOUSE_BUTTON_4;
//             else
//                 button = GLFW_MOUSE_BUTTON_5;
  
			if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
				msg == WM_MBUTTONDOWN || msg == WM_XBUTTONDOWN)
			{
				inputType = EInputType::InputPress;
			}
			else
			{
				inputType = EInputType::InputRelease;
			}

			for (int32_t i = +EInputKey::MouseLeft; i <= +EInputKey::MouseRight; i++)
			{
				//if (window->mouseButtons[i] == GLFW_PRESS)
					//break;
			}

			//if (i > GLFW_MOUSE_BUTTON_LAST)
				SetCapture(hwnd);

                ForwardKeyInput(inputKey, inputType);

// 			for (i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++)
// 			{
// 				if (window->mouseButtons[i] == GLFW_PRESS)
// 					break;
// 			}

			//if (i > GLFW_MOUSE_BUTTON_LAST)
				ReleaseCapture();

			if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONUP)
				return TRUE;

 			return 0;
 		}

		case WM_MOUSEMOVE:
		{
			//LOG_WINMSG(WM_MOUSEMOVE); // Spamming

			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
 
 			if (!InputSystem::Get().CursorsTracked)
 			{
 				TRACKMOUSEEVENT tme;
 				ZeroMemory(&tme, sizeof(tme));
 				tme.cbSize = sizeof(tme);
 				tme.dwFlags = TME_LEAVE;
 				tme.hwndTrack = hwnd;
 				TrackMouseEvent(&tme);
 
 				InputSystem::Get().CursorsTracked = true;
 			}
 
 			const InputSystem& inputSys = InputSystem::Get();
 			const ECursorMode cursorMode = inputSys.CurrentCursorMode;
			if (cursorMode == ECursorMode::Disabled)
			{
				if (!bReactToMouseInput || bRawMouseInput)
				{
					break;
					// TODO: Refactor this into pointer that keeps track of what window has pointer disabled
				}

				const int dx = x - InputSystem::Get().LastMousePos.x;
				const int dy = y - InputSystem::Get().LastMousePos.y;

                ForwardMouseMoveInput(InputSystem::Get().VirtualMousePos.x + dx, InputSystem::Get().VirtualMousePos.y + dy);
			}

			InputSystem::Get().LastMousePos.x = x;
			InputSystem::Get().LastMousePos.y = y;

			return 0;
		}

 		case WM_INPUT:
 		{
 			//LOG_WINMSG(WM_INPUT); // Spam

			UINT size = 0;
			HRAWINPUT ri = (HRAWINPUT)lParam;
			RAWINPUT* data = NULL;
			int dx, dy;

            static int rawInputSize = 0;
  
            if (!bReactToMouseInput || !bRawMouseInput)
            {
                break;
            }

			GetRawInputData(ri, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
			if (size > (UINT)rawInputSize)
			{
				free(rawInput);
				rawInput = (RAWINPUT*)calloc(size, 1);
				rawInputSize = size;
			}

			size = rawInputSize;
			if (GetRawInputData(ri, RID_INPUT,
				rawInput, &size,
				sizeof(RAWINPUTHEADER)) == (UINT)-1)
			{
				ASSERT_MSG(false, "Win32: Failed to retrieve raw input data");
				break;
			}

			data = rawInput;
			if (data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
			{
				dx = data->data.mouse.lLastX - InputSystem::Get().LastMousePos.x;
				dy = data->data.mouse.lLastY - InputSystem::Get().LastMousePos.y;
			}
			else
			{
				dx = data->data.mouse.lLastX;
				dy = data->data.mouse.lLastY;
			}

            ForwardMouseMoveInput(InputSystem::Get().VirtualMousePos.x + dx, InputSystem::Get().VirtualMousePos.y + dy);

			InputSystem::Get().LastMousePos.x += dx;
			InputSystem::Get().LastMousePos.y += dy;

            // TODO: Check if this shouldn't be reset at one moment

 			break;
 		}
 
 		case WM_MOUSELEAVE:
 		{
 			LOG_WINMSG(WM_MOUSELEAVE);
 			InputSystem::Get().CursorsTracked = false;

 //             _glfwInputCursorEnter(window, GLFW_FALSE);
 			return 0;
 		}
 
 		case WM_MOUSEWHEEL:
 		{
			const float SpinFactor = 1 / 120.0f;
			const SHORT WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

			POINT CursorPoint;
			CursorPoint.x = GET_X_LPARAM(lParam);
			CursorPoint.y = GET_Y_LPARAM(lParam);

			const glm::vec<2, int> CursorPos(CursorPoint.x, CursorPoint.y);
			const float Delta = WheelDelta * SpinFactor;

			ForwardMouseScrollInput(Delta, CursorPos);

 			LOG_WINMSG(WM_MOUSEWHEEL);
			//ForwardMouseScrollInput(InputSystem::Get().VirtualMousePos.x + dx, InputSystem::Get().VirtualMousePos.y + dy);

 			return 0;
 		}
 
 		case WM_MOUSEHWHEEL:
 		{
 			LOG_WINMSG(WM_MOUSEHWHEEL);
 			// This message is only sent on Windows Vista and later
 			// NOTE: The X-axis is inverted for consistency with macOS and X11
 			//_glfwInputScroll(window, -((SHORT) HIWORD(wParam) / (double) WHEEL_DELTA), 0.0);
 			return 0;
 		}
 
 		case WM_ENTERSIZEMOVE:
 		case WM_ENTERMENULOOP:
 		{
 			LOG_WINMSG(WM_ENTERSIZEMOVE - WM_ENTERMENULOOP);
 			if (InputSystem::Get().OngoingFrameAction)
 			{
 				break;
 			}
 			// 
 			//             // HACK: Enable the cursor while the user is moving or
 			//             //       resizing the window or using the window menu
 			if (InputSystem::Get().CurrentCursorMode == ECursorMode::Disabled)
 			{
 				EnableCursor(hwnd);
 			}
 
 			break;
 		}
 
 		case WM_EXITSIZEMOVE:
 		case WM_EXITMENULOOP:
 		{
 			LOG_WINMSG(WM_EXITSIZEMOVE - WM_EXITMENULOOP);
 			if (InputSystem::Get().OngoingFrameAction)
 				break;
 
 			// HACK: Disable the cursor once the user is done moving or
 			//       resizing the window or using the menu
 			if (InputSystem::Get().CurrentCursorMode == ECursorMode::Disabled)
 			{
 				DisableCursor(hwnd);
 			}
 
 			break;
 		}
 
 		case WM_SIZE:
 		{
 			LOG_WINMSG(WM_SIZE);
 			//             const int width = LOWORD(lParam);
 			//             const int height = HIWORD(lParam);
 			//             const GLFWbool iconified = wParam == SIZE_MINIMIZED;
 			//             const GLFWbool maximized = wParam == SIZE_MAXIMIZED ||
 			//                                        (window->win32.maximized &&
 			//                                         wParam != SIZE_RESTORED);
 			// 
 			//             if (_glfw.win32.disabledCursorWindow == window)
 			//                 updateClipRect(window);
 			// 
 			//             if (window->win32.iconified != iconified)
 			//                 _glfwInputWindowIconify(window, iconified);
 			// 
 			//             if (window->win32.maximized != maximized)
 			//                 _glfwInputWindowMaximize(window, maximized);
 			// 
 			//             if (width != window->win32.width || height != window->win32.height)
 			//             {
 			//                 window->win32.width = width;
 			//                 window->win32.height = height;
 			// 
 			//                 _glfwInputFramebufferSize(window, width, height);
 			//                 _glfwInputWindowSize(window, width, height);
 			//             }
 			// 
 			//             if (window->monitor && window->win32.iconified != iconified)
 			//             {
 			//                 if (iconified)
 			//                     releaseMonitor(window);
 			//                 else
 			//                 {
 			//                     acquireMonitor(window);
 			//                     fitToMonitor(window);
 			//                 }
 			//             }
 			// 
 			//             window->win32.iconified = iconified;
 			//             window->win32.maximized = maximized;
 			return 0;
 		}
 
 		case WM_MOVE:
 		{
 			LOG_WINMSG(WM_MOVE);
 
 			//             if (_glfw.win32.disabledCursorWindow == window)
 //                 updateClipRect(window);
 // 
 
 
 //             // NOTE: This cannot use LOWORD/HIWORD recommended by MSDN, as
 //             // those macros do not handle negative window positions correctly
 //             _glfwInputWindowPos(window,
 //                                 GET_X_LPARAM(lParam),
 //                                 GET_Y_LPARAM(lParam));
 			return 0;
 		}
 
 		case WM_SIZING:
 		{
 			LOG_WINMSG(WM_SIZING);
 			//if (window->numer == GLFW_DONT_CARE || window->denom == GLFW_DONT_CARE)
 			{
 				break;
 			}
 
 			//             applyAspectRatio(window, (int) wParam, (RECT*) lParam);
 			//             return TRUE;
 		}
 
 		case WM_GETMINMAXINFO:
 		{
 			LOG_WINMSG(WM_GETMINMAXINFO);
 			// 			int xoff, yoff;
 			// 			UINT dpi = USER_DEFAULT_SCREEN_DPI;
 			// 			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
 			// 
 			// 			if (window->monitor)
 			// 				break;
 			// 
 			// 			if (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32())
 			// 				dpi = GetDpiForWindow(window->win32.handle);
 			// 
 			//             getFullWindowSize(getWindowStyle(window), getWindowExStyle(window),
 			//                               0, 0, &xoff, &yoff, dpi);
 			// 
 			//             if (window->minwidth != GLFW_DONT_CARE &&
 			//                 window->minheight != GLFW_DONT_CARE)
 			//             {
 			//                 mmi->ptMinTrackSize.x = window->minwidth + xoff;
 			//                 mmi->ptMinTrackSize.y = window->minheight + yoff;
 			//             }
 			// 
 			//             if (window->maxwidth != GLFW_DONT_CARE &&
 			//                 window->maxheight != GLFW_DONT_CARE)
 			//             {
 			//                 mmi->ptMaxTrackSize.x = window->maxwidth + xoff;
 			//                 mmi->ptMaxTrackSize.y = window->maxheight + yoff;
 			//             }
 			// 
 			//             if (!window->decorated)
 			//             {
 			//                 MONITORINFO mi;
 			//                 const HMONITOR mh = MonitorFromWindow(window->win32.handle,
 			//                                                       MONITOR_DEFAULTTONEAREST);
 			// 
 			//                 ZeroMemory(&mi, sizeof(mi));
 			//                 mi.cbSize = sizeof(mi);
 			//                 GetMonitorInfo(mh, &mi);
 			// 
 			//                 mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
 			//                 mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
 			//                 mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
 			//                 mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
 			//             }
 
 			return 0;
 		}
 
 		case WM_PAINT:
 		{
 			LOG_WINMSG(WM_PAINT);
 
 			//_glfwInputWindowDamage(window);
 
 
 // 			PAINTSTRUCT ps;
 // 			HDC hdc;
 // 			hdc = BeginPaint(hwnd, &ps);
 // 			EndPaint(hwnd, &ps);
 			break;
 		}
 
 		case WM_ERASEBKGND:
 		{
 			LOG_WINMSG(WM_ERASEBKGND);
 			return TRUE;
 		}
 
 		case WM_NCACTIVATE:
 		case WM_NCPAINT:
 		{
 			LOG_WINMSG(WM_NCACTIVATE - WM_NCPAINT);
 			// Prevent title bar from being drawn after restoring a minimized
 			// undecorated window
 			//if (!window->decorated)
 				//return TRUE;
 
 			break;
 		}
 
 		case WM_DWMCOMPOSITIONCHANGED:
 		case WM_DWMCOLORIZATIONCOLORCHANGED:
 		{
 			LOG_WINMSG(WM_DWMCOMPOSITIONCHANGED - WM_DWMCOLORIZATIONCOLORCHANGED);
 			//if (window->win32.transparent)
 				//updateFramebufferTransparency(window);
 			return 0;
 		}
 
 		case WM_GETDPISCALEDSIZE:
 		{
 			LOG_WINMSG(WM_GETDPISCALEDSIZE);
 			//             if (window->win32.scaleToMonitor)
 //                 break;
 
 			// Adjust the window size to keep the content area size constant
 			//if (_glfwIsWindows10CreatorsUpdateOrGreaterWin32())
 			{
 				//                 RECT source = {0}, target = {0};
 				//                 SIZE* size = (SIZE*) lParam;
 				// 
 				//                 AdjustWindowRectExForDpi(&source, getWindowStyle(window),
 				//                                          FALSE, getWindowExStyle(window),
 				//                                          GetDpiForWindow(window->win32.handle));
 				//                 AdjustWindowRectExForDpi(&target, getWindowStyle(window),
 				//                                          FALSE, getWindowExStyle(window),
 				//                                          LOWORD(wParam));
 				// 
 				//                 size->cx += (target.right - target.left) -
 				//                             (source.right - source.left);
 				//                 size->cy += (target.bottom - target.top) -
 				//                             (source.bottom - source.top);
 				return TRUE;
 			}
 
 			break;
 		}
 
 		case WM_DPICHANGED:
 		{
 			LOG_WINMSG(WM_DPICHANGED);
 
 			// const float xscale = HIWORD(wParam) / (float) USER_DEFAULT_SCREEN_DPI;
 //             const float yscale = LOWORD(wParam) / (float) USER_DEFAULT_SCREEN_DPI;
 // 
 //             // Resize windowed mode windows that either permit rescaling or that
 //             // need it to compensate for non-client area scaling
 //             if (!window->monitor &&
 //                 (window->win32.scaleToMonitor ||
 //                  _glfwIsWindows10CreatorsUpdateOrGreaterWin32()))
 //             {
 //                 RECT* suggested = (RECT*) lParam;
 //                 SetWindowPos(window->win32.handle, HWND_TOP,
 //                              suggested->left,
 //                              suggested->top,
 //                              suggested->right - suggested->left,
 //                              suggested->bottom - suggested->top,
 //                              SWP_NOACTIVATE | SWP_NOZORDER);
 //             }
 // 
 //             _glfwInputWindowContentScale(window, xscale, yscale);
 			break;
 		}
 
 		case WM_SETCURSOR:
 		{
 			LOG_WINMSG(WM_SETCURSOR);
 
 			if (LOWORD(lParam) == HTCLIENT)
 			{
                UpdateCursorImage(hwnd);
 				return true;
 			}
 
 			break;
 		}
 
 		case WM_DROPFILES:
 		{
 			LOG_WINMSG(WM_DROPFILES);
 			//             HDROP drop = (HDROP) wParam;
 //             POINT pt;
 //             int i;
 // 
 //             const int count = DragQueryFileW(drop, 0xffffffff, NULL, 0);
 //             char** paths = calloc(count, sizeof(char*));
 // 
 //             // Move the mouse to the position of the drop
 //             DragQueryPoint(drop, &pt);
 //             _glfwInputCursorPos(window, pt.x, pt.y);
 // 
 //             for (i = 0;  i < count;  i++)
 //             {
 //                 const UINT length = DragQueryFileW(drop, i, NULL, 0);
 //                 WCHAR* buffer = calloc((size_t) length + 1, sizeof(WCHAR));
 // 
 //                 DragQueryFileW(drop, i, buffer, length + 1);
 //                 paths[i] = _glfwCreateUTF8FromWideStringWin32(buffer);
 // 
 //                 free(buffer);
 //             }
 // 
 //             _glfwInputDrop(window, count, (const char**) paths);
 // 
 //             for (i = 0;  i < count;  i++)
 //                 free(paths[i]);
 //             free(paths);
 // 
 //             DragFinish(drop);
 			return 0;
 		}
		}

		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
	const wchar_t CLASS_NAME[] = L"SecondEngineWindowClass";

	void* CreateWindowsWindow(const int32_t desiredWidth, const int32_t desiredHeight)
	{
		void* test = 0;
		SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0,
			&test, 0);
		SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, UIntToPtr(0),
			SPIF_SENDCHANGE);

		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		HINSTANCE instance = GetModuleHandle(nullptr);
		WNDCLASSW wc = {};
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc = WindowMessageLoop;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = instance;
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = 0;
		wc.lpszClassName = CLASS_NAME;

		unsigned short res = RegisterClassW(&wc);
		if (!res)
		{
			LOG_ERROR("Failed to register window class");
			ASSERT(0);
			return HWND(0);
		}

		// Compute window rectangle dimensions based on requested client area dimensions.
		RECT R = { 0, 0, desiredWidth, desiredHeight };
        DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX;

		AdjustWindowRect(&R, windowStyle, false);
		int32_t width = R.right - R.left;
		int32_t height = R.bottom - R.top;

		HWND newWindow = CreateWindowExW(
			0,
			CLASS_NAME,
			L"Main Window",
            windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, nullptr);

		ChangeWindowMessageFilterEx(newWindow,
			WM_DROPFILES, MSGFLT_ALLOW, NULL);
		ChangeWindowMessageFilterEx(newWindow,
			WM_COPYDATA, MSGFLT_ALLOW, NULL);
		ChangeWindowMessageFilterEx(newWindow,
			0x0049, MSGFLT_ALLOW, NULL);

		if (!newWindow)
		{
			ASSERT(0);

			return nullptr;
		}

		SetPropW(newWindow, L"WindowProp", (void*)1);

		return newWindow;
	}
}

eastl::wstring AnsiToWString(const char* ansiString)
{
	wchar_t buffer[512];
	Win32Call(MultiByteToWideChar(CP_ACP, 0, ansiString, -1, buffer, 512));
	return eastl::wstring(buffer);
}

eastl::string WStringToAnsi(const wchar_t* wideString)
{
	char buffer[512];
	Win32Call(WideCharToMultiByte(CP_ACP, 0, wideString, -1, buffer, 612, NULL, NULL));
	return eastl::string(buffer);
}
