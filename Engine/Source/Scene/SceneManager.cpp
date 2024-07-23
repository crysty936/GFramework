#include "SceneManager.h"
#include "Core/EngineUtils.h"
#include "Scene/Scene.h"
#include "Camera/Camera.h"
#include "Renderer/Drawable/ShapesUtils/BasicShapes.h"

SceneManager* SceneManager::Instance = nullptr;

SceneManager::SceneManager()
{
	// TODO Should be implemented to read the scene from a serialized output
	CurrentScene = eastl::make_unique<class Scene>();
}
SceneManager::~SceneManager() = default;

void SceneManager::Init()
{
	Instance = new SceneManager{};
}

void SceneManager::Terminate()
{
	ASSERT(Instance);

	delete Instance;
}

