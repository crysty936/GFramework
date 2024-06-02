#pragma once
#include "WindowProperties.h"
#include "InputSystem/CursorMode.h"
#include "InputSystem/InputKeys.h"
#include "InputSystem/InputType.h"

class WindowsWindow
{
public:
	WindowsWindow(const bool Init = true, const WindowProperties& inProperties = {});
	~WindowsWindow();

	void OnKeyInput(const EInputKey inKey, const EInputType inType);
	void SetVSyncEnabled(const bool inEnabled);
	inline void* GetHandle() const { return WindowHandle; }
	const WindowProperties& GetProperties() const { return Properties; }
	bool ShouldClose() const { return CloseRequested; }
	void RequestClose() { CloseRequested = true; }

private:
	void* WindowHandle;
	WindowProperties Properties;
	bool CloseRequested = false;
};