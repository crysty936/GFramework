#include "InputSystem/InputType.h"

const char* ToString(EInputType inEvent)
{
	switch (inEvent)
	{
	case EInputType::InputPress:
		return "InputPress";
		break;
	case EInputType::InputRelease:
		return "InputRelease";
		break;
	case EInputType::InputRepeat:
		return "InputRepeat";
		break;         
	default:
		return "Missing Input Type String";
	}
}
