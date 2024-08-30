#include "Editor.h"
#include "Controller/EditorController.h"
#include "Scene/SceneManager.h"
#include "Entity/TransformObject.h"
#include "Scene/Scene.h"
#include "Camera/Camera.h"
#include "Core/AppModeBase.h"
#include "imgui.h"

Editor* GEditor = nullptr;

void Editor::Init()
{
	GEditor = new Editor();
	GEditor->Internal_Init();
}

void Editor::Terminate()
{
	delete GEditor;
}

Editor::Editor()
{
	Controller = eastl::make_unique<EditorController>();
}

Editor::~Editor() = default;

void Editor::Tick(float inDeltaT)
{
	Controller->ExecuteCallbacks();
}

void Editor::Internal_Init()
{
	SceneManager& sManager = SceneManager::Get();
	Scene& currentScene = sManager.GetCurrentScene();
	ViewportCamera = currentScene.GetCurrentCamera();
	ViewportCamera->SetMovementDelegates(*Controller);


	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &Editor::MoveCameraLeft);
		EInputKey key1 = EInputKey::A;
		EInputKey key2 = EInputKey::Left;
		OnKeyAction action1 = { del, {}, key1, false };
		OnKeyAction action2 = { del, {}, key2, false };
		Controller->AddListener(action1);
		Controller->AddListener(action2);
	}

	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &Editor::MoveCameraRight);
		EInputKey key1 = EInputKey::D;
		EInputKey key2 = EInputKey::Right;
		OnKeyAction action1 = { del, {}, key1, false };
		OnKeyAction action2 = { del, {}, key2, false };

		Controller->AddListener(action1);
		Controller->AddListener(action2);
	}

	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &Editor::MoveCameraUp);
		EInputKey key1 = EInputKey::W;
		EInputKey key2 = EInputKey::Up;
		OnKeyAction action1 = { del, {}, key1, false };
		OnKeyAction action2 = { del, {}, key2, false };
		Controller->AddListener(action1);
		Controller->AddListener(action2);
	}

	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &Editor::MoveCameraDown);
		EInputKey key1 = EInputKey::S;
		EInputKey key2 = EInputKey::Down;
		OnKeyAction action1 = { del, {}, key1, false };
		OnKeyAction action2 = { del, {}, key2, false };

		Controller->AddListener(action1);
		Controller->AddListener(action2);
	}

	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &Editor::DebugCursorMode);
		EInputKey key = EInputKey::Q;
		OnKeyAction action = { del, {}, key, true };
		Controller->AddListener(action);
	}

	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &Editor::BoostCameraSpeed);
		EInputKey key = EInputKey::V;
		OnKeyAction action = { del, {}, key, true };
		Controller->AddListener(action);
	}

	{
		KeyActionDelegate mouseRightPressedDel = KeyActionDelegate::CreateRaw(this, &Editor::OnMouseRightPressed);
		KeyActionDelegate mouseRightReleasedDel = KeyActionDelegate::CreateRaw(this, &Editor::OnMouseRightReleased);

		EInputKey key = EInputKey::MouseRight;
		OnKeyAction action = { mouseRightPressedDel, mouseRightReleasedDel, key, true };
		Controller->AddListener(action);
	}
}

float CameraSpeed = 0.1f;

void Editor::MoveCameraLeft()
{
	if (!InViewportNavigateMode)
	{
		return;
	}

	ViewportCamera->Move(EMovementDirection::Left, CameraSpeed);
}

void Editor::MoveCameraRight()
{
	if (!InViewportNavigateMode)
	{
		return;
	}

	ViewportCamera->Move(EMovementDirection::Right, CameraSpeed);
}

void Editor::MoveCameraUp()
{
	if (!InViewportNavigateMode)
	{
		return;
	}

	ViewportCamera->Move(EMovementDirection::Forward, CameraSpeed);
}

void Editor::MoveCameraDown()
{
	if (!InViewportNavigateMode)
	{
		return;
	}

	ViewportCamera->Move(EMovementDirection::Back, CameraSpeed);
}

static bool mouseTest = false;

void Editor::DebugCursorMode()
{
	const ECursorMode mode = mouseTest ? ECursorMode::Enabled : ECursorMode::Disabled;
	InputSystem::Get().SetCursorMode(mode);

	mouseTest = !mouseTest;
}

void Editor::BoostCameraSpeed()
{
	static bool boostOn = false;

	if (boostOn)
	{
		CameraSpeed = 0.4f;
	}
	else
	{
		CameraSpeed = 10.f;
	}

	boostOn = !boostOn;
}

void Editor::OnMouseRightPressed()
{
	InputSystem::Get().SetCursorMode(ECursorMode::Disabled);
	ImGui::SetWindowFocus(nullptr);
	InViewportNavigateMode = true;
	LOG_INFO("Viewport navigate mode ON.");
}

void Editor::OnMouseRightReleased()
{
	// Check if we have successfully gone into viewport navigation mode
	if (!InViewportNavigateMode)
	{
		return;
	}

	InputSystem::Get().SetCursorMode(ECursorMode::Enabled);
	InViewportNavigateMode = false;
	LOG_INFO("Viewport navigate mode OFF.");
}

ControllerBase& Editor::GetController()
{
	return *Controller;
}


