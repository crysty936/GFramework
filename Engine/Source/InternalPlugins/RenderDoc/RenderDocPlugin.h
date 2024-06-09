#pragma once
#include "InternalPlugins/IInternalPlugin.h"
#include <stdint.h>
#include "windows.h"
#include "RenderDocApp.h"



class RenderDocPlugin : public IInternalPlugin
{
public:
	void Init() override;
	void Shutdown() override;
	void Tick(const float inDeltaTime) override;
	bool IsInit() override;

	void DoCapture();



private:
	void OnCapturePressed();
	void OnEngineInitDone();
	void LaunchRenderDoc();

private:
	HINSTANCE mRenderDocDLL = 0;
	UINT32 m_SocketPort = 0;
	RENDERDOC_API_1_0_0* RenderDocAPI = nullptr;

	void* GetRenderDocFunctionPointer(HINSTANCE ModuleHandle, LPCSTR FunctionName);


private:
	bool bIsInit = false;
	bool bPendingCapture = false;
	bool bCaptureInProgress = false;
	uint64_t CaptureEndFrame = 0;
};
