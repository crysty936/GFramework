#include "Window/WindowsWindow.h"
#include "Logger/Logger.h"
#include <windows.h>
#include "Core/WindowsPlatform.h"
#include "InputSystem/InputSystem.h"

WindowsWindow::WindowsWindow(const bool Init, const WindowProperties& inProperties /*= {}*/)
	: Properties{ inProperties }
{
	if (!Init)
	{
		return;
	}

	WindowHandle = WindowsPlatform::CreateWindowsWindow(inProperties.Width, inProperties.Height);

	ShowWindow(static_cast<HWND>(WindowHandle), SW_SHOW);
	UpdateWindow(static_cast<HWND>(WindowHandle));

	InputSystem::Get().GetOnKeyInputDel().BindRaw(this, &WindowsWindow::OnKeyInput);
}

WindowsWindow::~WindowsWindow()
{
	RemovePropW(static_cast<HWND>(WindowHandle), L"WindowProp");
	DestroyWindow(static_cast<HWND>(WindowHandle));
}

void WindowsWindow::OnKeyInput(const EInputKey inKey, const EInputType inType)
{
	if (inKey == EInputKey::Escape)
	{
		CloseRequested = true;
	}
}

void WindowsWindow::SetVSyncEnabled(const bool inEnabled)
{
	Properties.VSyncEnabled = inEnabled;
	ASSERT(FALSE); // Not working with generic renderer
	//RHI->SetVSyncEnabled(inEnabled);
}


