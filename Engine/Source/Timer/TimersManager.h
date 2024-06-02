#pragma once
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "TimerBase.h"
#include "Core/EngineUtils.h"

class TimersManager
{
public:
	static void Init();
	static void Terminate();
	static inline TimersManager& Get() { ASSERT(Instance); return *Instance; }

public:
	void AddTimer(eastl::shared_ptr<TimerBase> inTimer);
	void TickTimers(float inDeltaT);

private:
	TimersManager();
	~TimersManager();

private:
	static TimersManager* Instance;
	eastl::vector<eastl::shared_ptr<TimerBase>> Timers;
};