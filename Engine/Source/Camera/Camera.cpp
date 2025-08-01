#include "Camera/Camera.h"
#include "Controller/ControllerBase.h"
#include "Logger/Logger.h"
#include "Entity/TransformObject.h"
#include "Editor/Editor.h"
#include "Window/WindowProperties.h"
#include "Window/WindowsWindow.h"
#include "Math/MathUtils.h"

Camera::Camera()
	: Entity("Camera") 
{}

Camera::~Camera() = default;

// TODO: Make these customizable
const float CAMERA_FOV = 45.f;
const float CAMERA_NEAR = 0.1f;
const float CAMERA_FAR = 500.f;

void Camera::Init()
{
	const WindowsWindow& mainWindow = GEngine->GetMainWindow();
	const WindowProperties& props = mainWindow.GetProperties();

	//DirectX::XMMATRIX testMatrix = DirectX::XMMatrixPerspectiveFovLH(glm::radians(CAMERA_FOV), static_cast<float>(props.Width) / static_cast<float>(props.Height), CAMERA_NEAR, CAMERA_FAR);

	ProjMatCache = glm::perspectiveLH_ZO(glm::radians(CAMERA_FOV), static_cast<float>(props.Width) / static_cast<float>(props.Height), CAMERA_NEAR, CAMERA_FAR);
}

void Camera::Tick(const float inDeltaT)
{
	
}

void Camera::Move(EMovementDirection inDirection, const float inSpeed) 
{
	constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	const glm::quat absRotation = GetAbsoluteTransform().Rotation;

	//glm::vec3 front = glm::normalize(absRotation * glm::vec3(0.f, 0.f, -1.f)); // Right handed, for OpenGl
	glm::vec3 front = glm::normalize(absRotation * glm::vec3(0.f, 0.f, 1.f));

	//glm::vec3 right = glm::normalize(glm::cross(front, up)); // For OpenGl
	glm::vec3 right = glm::normalize(glm::cross(up, front));

	front *= MouseMoveSensitivity;
	right *= MouseMoveSensitivity;

	if (TransformObjPtr parentShared = Parent.lock())
	{
		glm::vec3 movementVector(0.f);

		switch (inDirection)
		{
		case EMovementDirection::Forward:
			movementVector = front * inSpeed;
			break;
		case EMovementDirection::Back:
			movementVector -= front * inSpeed;
			break;
		case EMovementDirection::Right:
			movementVector = right * inSpeed;
			break;
		case EMovementDirection::Left:
			movementVector -= right * inSpeed;
			break;
		case EMovementDirection::Down:
			movementVector -= up * inSpeed;
			break;
		case EMovementDirection::Up:
			movementVector += up * inSpeed;
			break;
		default:
			break;
		}

		parentShared->Move(movementVector);
	}
}


void Camera::SetMovementDelegates(ControllerBase& inController)
{
	inController.OnMouseMoved().BindRaw(this, &Camera::OnMousePosChanged);
	inController.OnMouseScroll().BindRaw(this, &Camera::OnMouseScrollChanged);
}

void Camera::OnMouseScrollChanged(const float inNewY)
{
	if (GEditor->IsViewportNavigateModeEnabled())
	{
		MouseMoveSensitivity = glm::max(0.01f, MouseMoveSensitivity + inNewY);
	}
}

void Camera::OnMousePosChanged(const float inNewYaw, const float inNewPitch)
{
	if (FirstMouse)
	{
		FirstMouse = false;

		MouseLastYaw = inNewYaw;
		MouseLastPitch = inNewPitch;

		return;
	}

	const float yawOffset = (inNewYaw - MouseLastYaw) * MouseLookSensitivity;
	const float pitchOffset = ((-1.f) * (inNewPitch - MouseLastPitch)) * MouseLookSensitivity;


	MouseLastYaw = inNewYaw;
	MouseLastPitch = inNewPitch;

// 	LOG_INFO("Mouse yaw offset %f", yawOffset);
// 	LOG_INFO("Mouse yaw %f", inNewYaw);

 	if (TransformObjPtr parentShared = Parent.lock())
 	{
 		//parentShared->Rotate(-yawOffset, glm::vec3(0.f, 1.f, 0.f)); // For OpenGl
 		parentShared->Rotate(yawOffset, glm::vec3(0.f, 1.f, 0.f));
 	}

	//Rotate(pitchOffset, glm::vec3(1.f, 0.f, 0.f));// For OpenGl
	Rotate(-pitchOffset, glm::vec3(1.f, 0.f, 0.f));
}

glm::mat4 Camera::GetLookAt()
{
// 	//https://www.3dgep.com/understanding-the-view-matrix/

  	constexpr glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
 	const Transform& absTransf = GetAbsoluteTransform();
  	const glm::vec3 cameraDir = glm::normalize(absTransf.Rotation * glm::vec3(0.f, 0.f, 1.f)); // Extract Z column
	const glm::mat4 lookAt = MathUtils::BuildLookAt(cameraDir, absTransf.Translation);

	return lookAt;

	//const glm::mat4 viewMatrix = glm::inverse(GetAbsoluteTransform().GetMatrix());
 	//return viewMatrix;
}

float Camera::GetNear() const
{
	return CAMERA_NEAR;
}

float Camera::GetFar() const
{
	return CAMERA_FAR;
}

float Camera::GetFOV() const
{
	return CAMERA_FOV;
}

