#include "GameController.h"
#include "Core/EngineCore.h"

GameController::GameController()
{
	InputSystem::Get().GetOnKeyInputDel().BindRaw(this, &GameController::OnKeyInputReceived);
	InputSystem::Get().GetOnMouseMovedDel().BindRaw(this, &GameController::OnMouseMoveInputReceived);
	InputSystem::Get().GetOnMouseScrollDel().BindRaw(this, &GameController::OnMouseScrollInputReceived);
}

GameController::~GameController() = default;
