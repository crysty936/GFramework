#include "EditorController.h"
#include "Core/AppCore.h"

EditorController::EditorController()
{
	InputSystem::Get().GetOnKeyInputDel().BindRaw(this, &EditorController::OnKeyInputReceived);
	InputSystem::Get().GetOnMouseMovedDel().BindRaw(this, &EditorController::OnMouseMoveInputReceived);
	InputSystem::Get().GetOnMouseScrollDel().BindRaw(this, &EditorController::OnMouseScrollInputReceived);
}

EditorController::~EditorController() = default;

void EditorController::ExecuteCallbacks()
{
	// TODO [Editor-Game Separation]
	// If game started -> forward input to game controller
	// Else execute editor callbacks

	__super::ExecuteCallbacks();
}
