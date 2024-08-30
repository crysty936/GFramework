#pragma once
#include "EASTL/shared_ptr.h"
#include "InputSystem/InputDelegates.h"

 class Editor
 {
 public:
 	static void Init();
 	static void Terminate();

 public:
     void Tick(float inDeltaT);
 
	 class ControllerBase& GetController();

     inline bool IsViewportNavigateModeEnabled() { return InViewportNavigateMode; }

 private:
     Editor();
     ~Editor();

     void Internal_Init();

     void MoveCameraLeft();
     void MoveCameraRight();
     void MoveCameraUp();
     void MoveCameraDown();
     void DebugCursorMode();
     void BoostCameraSpeed();
     void OnMouseRightPressed();
     void OnMouseRightReleased();


 private:
	eastl::unique_ptr<class EditorController> Controller;
	eastl::shared_ptr<class Camera> ViewportCamera;
    bool InViewportNavigateMode = false;

 };
 
extern class Editor* GEditor; 
