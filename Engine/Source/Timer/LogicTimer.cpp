#include "LogicTimer.h"

LogicTimer::LogicTimer(float inDeltaT, TimerLogicDelegate inOnLogic)
	:TimerBase{inDeltaT}, OnLogicDelegate{inOnLogic}
{

}

LogicTimer::~LogicTimer() = default;

void LogicTimer::Tick(float inDeltaT)
{
	TimerBase::Tick(inDeltaT);

	OnLogicDelegate.Execute(inDeltaT, GetTimeLeft());
}

