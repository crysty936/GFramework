#include "ControllerBase.h"
#include "Core/AppCore.h"

ControllerBase::ControllerBase() = default;
ControllerBase::~ControllerBase() = default;

void ControllerBase::ExecuteCallbacks()
{
	if (bMouseScrolled)
	{
		bMouseScrolled = false;

		OnMouseScrollDelegate.Invoke(NewMouseScrollOffset);
	}

	if (bMouseMoved)
	{
		bMouseMoved = false;

		OnMouseMovedDelegate.Invoke(NewYaw, NewPitch);
	}

	for (const OnKeyAction& listener : KeyListeners)
	{
		if (KeyStates[listener.RequiredKey] == EInputType::InputPress || KeyStates[listener.RequiredKey] == EInputType::InputRepeat)
		{
			if (listener.Once)
			{
				if (KeyStates[listener.RequiredKey] == EInputType::InputPress)
				{
					// Declare it repeat until it's released
					KeyStates[listener.RequiredKey] = EInputType::InputRepeat;

					listener.Del.Execute();
				}

				return;
			}

			listener.Del.Execute();
		}
		else if (KeyStates[listener.RequiredKey] == EInputType::InputRelease)
		{
			if (listener.ReleasedDel.IsBound())
			{
				listener.ReleasedDel.Execute();
			}
		}
	}

	// Consume all Input Release states
	eastl::unordered_map<EInputKey, EInputType>::iterator it = KeyStates.begin();
	while (it != KeyStates.end())
	{
		if (it->second == EInputType::InputRelease)
		{
			it->second = EInputType::None;
		}

		++it;
	}
}

void ControllerBase::AddListener(OnKeyAction& inKeyAction)
{
	KeyListeners.push_back(inKeyAction);
}

void ControllerBase::OnKeyInputReceived(EInputKey inKeyCode, EInputType inEventType)
{
//  	if (inKeyCode == EInputKey::Escape)
//  	{
//  		Engine->StopEngine();
//  	}

#if 0
	switch (inEventType)
	{
	case EInputType::InputPress:
	 	LOG_INFO("Press detected");
	 	break;
	 
	case EInputType::InputRelease:
	 	LOG_INFO("Release detected");
	 	break;
	 
	case EInputType::InputRepeat:
	 	LOG_INFO("Repeat detected");
	 	break;
	}
#endif



	KeyStates[inKeyCode] = inEventType;
}

void ControllerBase::OnMouseMoveInputReceived(const float inNewYaw, const float inNewPitch)
{
	bMouseMoved = true;
	NewYaw = inNewYaw;
	NewPitch = inNewPitch;
}

void ControllerBase::OnMouseScrollInputReceived(const float inNewYOffset)
{
	bMouseScrolled = true;
	NewMouseScrollOffset = inNewYOffset;
}

