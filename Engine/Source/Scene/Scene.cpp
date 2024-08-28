#include "Scene.h"
#include "Entity/Entity.h"
#include "imgui.h"
#include "Camera/Camera.h"

Scene::Scene()
{
	CurrentCamera = eastl::make_shared<Camera>();

	// !Trying to copy the gameplay engine tactic to see if it works
	EntityPtr CameraParent = eastl::make_shared<Entity>("Camera Parent");
	CameraParent->AddChild(GetCurrentCamera());
	this->AddObject(CameraParent);
}

Scene::~Scene() = default;

void Scene::TickObjects(float inDeltaT)
{
	RecursivelyTickObjects(inDeltaT, Objects);
}

void Scene::InitObjects()
{
	RecursivelyInitObjects(Objects);
}

void Scene::AddObject(TransformObjPtr inObj)
{
	Objects.push_back(inObj);
}

void Scene::ImGuiDisplaySceneTree()
{
	ImGui::Begin("Scene");

	ImGuiRecursivelyDisplaySceneTree(Objects, true);

	ImGui::End();
}

void Scene::RecursivelyTickObjects(float inDeltaT, eastl::vector<TransformObjPtr>& inObjects)
{
	for (TransformObjPtr& obj : inObjects)
	{
		RecursivelyTickObjects(inDeltaT, obj->Children);

		// TODO: Place different objects in different lists, waste of cycles to do dynamic cast on all each frame
		if (Entity* entt = dynamic_cast<Entity*>(obj.get()))
		{
			entt->Tick(inDeltaT);
		}
	}
}

void Scene::RecursivelyInitObjects(eastl::vector<TransformObjPtr>& inObjects)
{
	for (TransformObjPtr& obj : inObjects)
	{
		if (Entity* entt = dynamic_cast<Entity*>(obj.get()))
		{
			entt->Init();
		}

		RecursivelyInitObjects(obj->Children);
	}
}

void Scene::ImGuiRecursivelyDisplaySceneTree(eastl::vector<TransformObjPtr>& inObjects, const bool inDisplayNode)
{
	for (TransformObjPtr& obj : inObjects)
	{
		bool displayChildNodes = false;
		if (inDisplayNode)
		{
			displayChildNodes = ImGui::TreeNode(obj->Name.c_str());

			if (displayChildNodes)
			{
				bool dirty = false;
				constexpr float dragSpeed = 0.05f;

				dirty |= ImGui::DragFloat3("Position", &obj->Location.x, dragSpeed);
				dirty |= ImGui::DragFloat3("Rotation", &obj->Rotation.x, dragSpeed);
				dirty |= ImGui::DragFloat3("Scale", &obj->Scale.x, dragSpeed);

				if (dirty)
				{
					obj->MakeTransfDirty();
				}
			}
		}

		ImGuiRecursivelyDisplaySceneTree(obj->Children, displayChildNodes);

		if (displayChildNodes)
		{
			ImGui::TreePop();
		}
	}
}
