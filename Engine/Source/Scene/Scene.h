#pragma once
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "Entity/TransformObject.h"
#include "Renderer/Drawable/ShapesUtils/BasicShapes.h"
#include "Renderer/RenderUtils.h"
#include "Camera/Camera.h"

/**
 * Scene graph
 * 
 */

class Scene
{
public:
	Scene();
	~Scene();

	void TickObjects(float inDeltaT);
	void InitObjects();
	void AddObject(TransformObjPtr inObj);
	
	void ImGuiDisplaySceneTree();

	inline eastl::shared_ptr<Camera>& GetCurrentCamera();
	inline const eastl::shared_ptr<Camera>& GetCurrentCamera() const;
	inline glm::mat4 GetMainCameraLookAt() const;

	//inline void AddLight(const eastl::shared_ptr<LightSource>& inLightData);
	//inline const eastl::vector<eastl::shared_ptr<LightSource>>& GetLights() const;


	inline const eastl::vector<TransformObjPtr>& GetAllObjects() const { return Objects; }

private:
	void RecursivelyTickObjects(float inDeltaT, eastl::vector<TransformObjPtr>& inObjects);
	void RecursivelyInitObjects(eastl::vector<TransformObjPtr>& inObjects);
	void ImGuiRecursivelyDisplaySceneTree(eastl::vector<TransformObjPtr>& inObjects, const bool inDisplayNode);

private:
	eastl::vector<TransformObjPtr> Objects;
	eastl::shared_ptr<Camera> CurrentCamera;
	//eastl::vector<eastl::shared_ptr<LightSource>> Lights;
};


eastl::shared_ptr<Camera>& Scene::GetCurrentCamera()
{
	return CurrentCamera;
}

const eastl::shared_ptr<Camera>& Scene::GetCurrentCamera() const
{
	return CurrentCamera;
}


glm::mat4 Scene::GetMainCameraLookAt() const
{
	return CurrentCamera->GetLookAt();
}


//void Scene::AddLight(const eastl::shared_ptr<LightSource>& inLightData)
//{
//	Lights.push_back(inLightData);
//}

//const eastl::vector<eastl::shared_ptr<LightSource>>& Scene::GetLights() const
//{
//	return Lights;
//}
