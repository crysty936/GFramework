#include "TimerBase.h"

TimerBase::TimerBase(float inTotalTime)
	: TimeLeft{inTotalTime}
{

}

TimerBase::~TimerBase() = default;

void TimerBase::Start()
{

}

void TimerBase::Tick(float inDeltaT)
{
	TimeLeft -= inDeltaT;
}

void TimerBase::End()
{

}

