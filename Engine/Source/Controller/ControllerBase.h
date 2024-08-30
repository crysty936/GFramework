#pragma once

#include "InputSystem/InputSystem.h"
#include "EASTL/unordered_map.h"
#include "InputSystem/InputKeys.h"
#include "InputSystem/InputType.h"
#include "EventSystem/EventSystem.h"

using KeyActionDelegate = Delegate<void>;

struct OnKeyAction
{
	KeyActionDelegate Del{};
	KeyActionDelegate ReleasedDel{};
	EInputKey RequiredKey{};
	bool Once{ false };
};

class ControllerBase
{
public:
	ControllerBase();
	virtual ~ControllerBase();

	virtual void ExecuteCallbacks();
	void AddListener(OnKeyAction& inKeyAction);
	inline MouseScrollDelegate& OnMouseScroll() { return OnMouseScrollDelegate; }
	inline MousePosDelegate& OnMouseMoved() { return OnMouseMovedDelegate; }

protected:
	void OnKeyInputReceived(EInputKey inKeyCode, EInputType inEventType);
	void OnMouseMoveInputReceived(const float inNewYaw, const float inNewPitch);
	void OnMouseScrollInputReceived(const float inNewYOffset);

private:
	eastl::unordered_map<EInputKey, EInputType> KeyStates{};
	eastl::vector<OnKeyAction> KeyListeners{};
	MousePosDelegate OnMouseMovedDelegate{};
	MouseScrollDelegate OnMouseScrollDelegate{};
	bool bMouseMoved{false};
	bool bMouseScrolled{false};
	float NewPitch{0.0f};
	float NewYaw{0.0f};
	float NewMouseScrollOffset{ 0.0f };
};