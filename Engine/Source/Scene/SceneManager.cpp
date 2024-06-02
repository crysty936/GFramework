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

	// !Trying to copy the gameplay engine tactic to see if it works
	EntityPtr CameraParent = eastl::make_shared<Entity>("Camera Parent");
	CameraParent->AddChild(CurrentScene->GetCurrentCamera());
	CurrentScene->AddObject(CameraParent);
}
SceneManager::~SceneManager() = default;

void SceneManager::Init()
{
	Instance = new SceneManager{};
}

void SceneManager::LoadScene()
{
	// TODO Should be implemented to read the scene from a serialized output
}

void SceneManager::Terminate()
{
	ASSERT(Instance);

	delete Instance;
}

