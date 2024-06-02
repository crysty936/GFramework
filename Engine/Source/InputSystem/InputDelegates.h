#pragma once
#include "EventSystem/EventSystem.h"
#include "InputKeys.h"
#include "InputType.h"

using KeyDelegate = MulticastDelegate<EInputKey, EInputType>;
using MousePosDelegate = MulticastDelegate<float, float>;
using MouseScrollDelegate = MulticastDelegate<float>;
