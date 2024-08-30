#include "InputSystem/InputSystem.h"
#include "Core/WindowsPlatform.h"
#include "Window/WindowsWindow.h"
#include "Core/AppCore.h"

InputSystem* InputSystem::Instance;

InputSystem::InputSystem() = default;
InputSystem::~InputSystem() = default;

void InputSystem::KeyCallback(EInputKey inKey, EInputType inAction)
{
	InputSystem& instance = InputSystem::Get();

	instance.OnKeyInputDelegate.Invoke(inKey, inAction);
	//LOG_INFO("Key input received for key %d with action: %s", static_cast<int16_t>(inKey), ToString(inAction));
}

void InputSystem::MousePosChangedCallback(const double inNewYaw, const double inNewPitch)
{
	const float yawFloat = static_cast<float>(inNewYaw);
	const float pitchFloat = static_cast<float>(inNewPitch);

	Instance->OnMouseMovedDelegate.Invoke(yawFloat, pitchFloat);
	//LOG_INFO("Mouse Pos Changed Callback with Yaw: %f and Pitch: %f", inNewYaw, inNewPitch);
}

void InputSystem::MouseScrollCallback(double xoffset, double yoffset)
{
	const float yFloat = static_cast<float>(yoffset);

	Instance->OnMouseScrollDelegate.Invoke(yFloat);
}

void InputSystem::Init()
{
	Instance = new InputSystem{};
}

void InputSystem::Terminate()
{
	ASSERT(Instance);

	delete(Instance);
}

void InputSystem::PollEvents()
{
	WindowsPlatform::PoolMessages();
}

KeyDelegate& InputSystem::GetOnKeyInputDel()
{
	return OnKeyInputDelegate;
}

MousePosDelegate& InputSystem::GetOnMouseMovedDel()
{
	return OnMouseMovedDelegate;
}

MouseScrollDelegate& InputSystem::GetOnMouseScrollDel()
{
	return OnMouseScrollDelegate;
}

void InputSystem::SetCursorMode(const ECursorMode inMode, void* inWindowHandle)
{
	if (!inWindowHandle)
	{
		inWindowHandle = GEngine->GetMainWindow().GetHandle();
	}

	CurrentCursorMode = inMode;
	WindowsPlatform::SetCursorMode(inWindowHandle, inMode);
}

void InputSystem::OnKeyPressedLog(EInputKey inKeyCode, EInputType inEventType)
{
	//LOG_INFO("Key input received for key %d with action: %s", static_cast<int32_t>(inKeyCode), ToString(inEventType));
}
