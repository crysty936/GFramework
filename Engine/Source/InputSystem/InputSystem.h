#pragma once
#include "EventSystem/EventSystem.h"
#include "Core/EngineUtils.h"
#include "InputSystem/InputKeys.h"
#include "InputSystem/InputType.h"
#include "Entity/Entity.h"
#include "CursorMode.h"
#include "Core/WindowsPlatform.h"
#include "Core/AppCore.h"
#include "InputDelegates.h"

class InputSystem
{
private:
	InputSystem();
	~InputSystem();

public:
	static void Init();
	static void Terminate();
	void PollEvents();

public:
	static inline InputSystem& Get() { ASSERT(Instance); return *Instance; }

	/** Callbacks for others to tie into */
	KeyDelegate& GetOnKeyInputDel();
	MousePosDelegate& GetOnMouseMovedDel();
	MouseScrollDelegate& GetOnMouseScrollDel();

	void SetCursorMode(const ECursorMode inMode, void* inWindowHandle = nullptr);

	bool OngoingFrameAction = false;
	bool CursorsTracked = false;
	ECursorMode CurrentCursorMode = ECursorMode::Enabled;
	glm::vec<2, int> CurrentCursorPos = {};
	glm::vec<2, int> LastMousePos = {};
	glm::vec<2, double> VirtualMousePos = {};

private:
	void OnKeyPressedLog(EInputKey inKeyCode, EInputType inEventType);
	static void KeyCallback(EInputKey inKey, EInputType inAction);
	static void MousePosChangedCallback(const double inNewYaw, const double inNewPitch);
	static void MouseScrollCallback(double xoffset, double yoffset);

	// Specialized versions for the editor layer
// 	KeyDelegate& Editor_GetOnKeyInputDel();
// 	MousePosDelegate& Editor_GetOnMouseMovedDel();
// 	MouseScrollDelegate& Editor_GetOnMouseScrollDel();

private:
	static InputSystem* Instance;
	static uint16_t KeyCodes[512];

private:
	KeyDelegate OnKeyInputDelegate;
	MousePosDelegate OnMouseMovedDelegate;
	MouseScrollDelegate OnMouseScrollDelegate;

	friend void WindowsPlatform::ForwardKeyInput(const EInputKey inKey, const EInputType inAction);
	friend void WindowsPlatform::ForwardMouseMoveInput(double inNewYaw, double inNewPitch);
	friend void WindowsPlatform::ForwardMouseScrollInput(float inWheelDelta, const glm::vec<2, int> inCursorPos);

	friend class Editor;
};
