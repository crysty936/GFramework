#pragma once

class GameModeBase
{
public:
	GameModeBase();
	virtual ~GameModeBase();

public:

	static GameModeBase* Get() { return GameMode; }

	// Scene setup (loading as well until specific implementation)
	virtual void Init();

	// Game Mode Tick is Run after all other objects
	virtual void Tick(float inDeltaT);

private:
	static GameModeBase* GameMode;
};