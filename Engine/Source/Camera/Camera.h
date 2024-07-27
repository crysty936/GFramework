#pragma once
#include "Entity/Entity.h"
#include "EASTL/shared_ptr.h"

enum class EMovementDirection
{
	Forward,
	Back,
	Left,
	Right
};

class Camera : public Entity
{
public:
	Camera();
	virtual ~Camera();

	virtual void Init() override;
	virtual void Tick(const float inDeltaT) override;
	
	void Move(EMovementDirection inDirection, const float inSpeed = 0.1f);
	void SetMovementDelegates(class ControllerBase& inController);
	void OnMouseScrollChanged(const float inNewY);
	glm::mat4 GetLookAt();

private:
	void OnMousePosChanged(const float inNewYaw, const float inNewPitch);
	
private:
	bool FirstMouse{true};
	float MouseLastYaw{};
	float MouseLastPitch{};
	float Yaw{ 0.f };
	float Pitch{ 0.f };
	float MouseLookSensitivity{ 0.5f };
	float MouseMoveSensitivity{ 1.0f };
};


// TODO: Implement raycasts
