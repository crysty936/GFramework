#pragma once

class AppModeBase
{
public:
	AppModeBase();
	virtual ~AppModeBase();

public:

	static AppModeBase* Get() { return GameMode; }

	virtual void BeginFrame();
	virtual void Draw();
	virtual void EndFrame();

	// Scene setup (loading as well until specific implementation)
	virtual void Init();
	static void Terminate();

	// Game Mode Tick is Run after all other objects
	virtual void Tick(float inDeltaT);

private:
	static AppModeBase* GameMode;
};