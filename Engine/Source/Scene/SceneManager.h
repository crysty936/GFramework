#pragma once
#include "EASTL/unique_ptr.h"
#include "EASTL/shared_ptr.h"

/**
 * Handles Scene loading, management, delivers current scene to renderer
 */
class SceneManager
{
private:
	SceneManager();
	virtual ~SceneManager();

public:
	static void Init();
	static void Terminate();

	static inline SceneManager& Get() { return *Instance; }

	inline class Scene& GetCurrentScene() const { return *CurrentScene; }

private:
	static SceneManager* Instance;
	eastl::unique_ptr<class Scene> CurrentScene;
};