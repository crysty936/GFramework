#include "Camera/Camera.h"
#include "Controller/ControllerBase.h"
#include "glm/gtc/type_ptr.hpp"
#include "Logger/Logger.h"
#include "Entity/TransformObject.h"
#include "Editor/Editor.h"

Camera::Camera()
	: Entity("Camera") 
{}

Camera::~Camera() = default;

void Camera::Init()
{

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
  	glm::vec3 cameraDir = glm::normalize(absTransf.Rotation * glm::vec3(0.f, 0.f, 1.f));

   	glm::vec3 right = glm::normalize(glm::cross(globalUp, cameraDir));
   	//glm::vec3 right = glm::normalize(glm::cross(cameraDir, globalUp)); // For Opengl(right-handed)
   	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDir, right));
	//glm::vec3 cameraUp = glm::normalize(glm::cross(right, cameraDir)); // For Opengl(right-handed)

	// Rotate inversely related to camera
//  	// Rotation Matrix
//  	/**
//  	* Normally this matrix would be like this:
//  	  	float firstMatrixA[16] =
//   		{
//   			right.x,		right.y,		right.z,		0,
//   			cameraUp.x,		cameraUp.y,		cameraUp.z,		0,
//   			cameraDir.x,	cameraDir.y,	cameraDir.z,		0,
//   			0,				0,				0,				1
//   		};
//  		
//  		And we would need to do:
//  		//rotationMatrix = glm::transpose(rotationMatrix);
//  		Because it's transpose is equal to it's inverse because it the matrix
//  		is orthonormalized
//  
//  		However, we can build it directly transposed
//  	 */

   	float firstMatrixA[16] =
   	{
 		right.x,		cameraUp.x,		cameraDir.x,		0,
 		right.y,		cameraUp.y,		cameraDir.y,		0,
 		right.z,		cameraUp.z,		cameraDir.z,		0,
   		0,				0,				0,				1
   	};
   

   	glm::mat4 rotationMatrix = glm::mat4(1.f);
   	memcpy(glm::value_ptr(rotationMatrix), firstMatrixA, sizeof(glm::mat4));

	// Move inversely related to camera
//  
//  	// Translation Matrix
//  	// Position is negated so that camera is at origin
//  	// 
//  	// Translation inverse is equal to T(-v) T(v)^-1 == T(-v)
// 
// 	//Also, inverse of a TranslationRotation matrix (X)^-1 = (T(t) * R)^-1 = R^t * T(-t), so R^-1 * T^-1, open parantheses mean inverse of operations and inverse of each
  	const glm::vec3 translation = absTransf.Translation;

 	float SecondMatrixA[16] =
  	{
  		1,		0,		0,		0,
  		0,		1,		0,		0,
  		0,		0,		1,		0,
  		-translation.x,		-translation.y,		-translation.z,		1	// For D3D12
  		//-translation.x,		-translation.y,		translation.z,		1	// For OpenGl(right-handed)
  	};

  	glm::mat4 translationMatrix = glm::mat4(1.f);
  	memcpy(glm::value_ptr(translationMatrix), SecondMatrixA, sizeof(glm::mat4));

  	glm::mat4 lookAt = rotationMatrix * translationMatrix;// In the shader it's pre-multiplied, meaning that it's first translated and then rotated with camera as center
 
 	return lookAt;


	//const glm::mat4 viewMatrix = glm::inverse(GetAbsoluteTransform().GetMatrix());
 	//return viewMatrix;
}

