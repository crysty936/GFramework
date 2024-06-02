#include "TimersManager.h"

TimersManager* TimersManager::Instance = nullptr;

TimersManager::TimersManager() = default;
TimersManager::~TimersManager() = default;

void TimersManager::Init()
{
	Instance = new TimersManager{};
}

void TimersManager::Terminate()
{
	Get().Timers.clear();
}

void TimersManager::AddTimer(eastl::shared_ptr<TimerBase> inTimer)
{
	Timers.push_back(inTimer);

	inTimer->Start();
}

void TimersManager::TickTimers(float inDeltaT)
{
	using timerIterator = eastl::shared_ptr<TimerBase>*;
	eastl::vector<timerIterator> toRemove;

	for (timerIterator timerIter = Timers.begin(); timerIter != Timers.end(); ++timerIter)
	{
		eastl::shared_ptr<TimerBase> timer = *timerIter;

		if (timer->GetTimeLeft() < 0.f)
		{
			timer->End();

			toRemove.push_back(timerIter);

			continue;
		}

		timer->Tick(inDeltaT);
	}

	for (timerIterator iter : toRemove)
	{
		Timers.erase(iter);
	}
}
