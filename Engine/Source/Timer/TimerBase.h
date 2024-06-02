#pragma once

class TimerBase
{
	
public:
	TimerBase(float inTotalTime);
	virtual ~TimerBase();

protected:
	virtual void Start();
	virtual void Tick(float inDeltaT);
	virtual void End();
	inline float GetTimeLeft() { return TimeLeft; }

private:
	float TimeLeft;

	friend class TimersManager;
};