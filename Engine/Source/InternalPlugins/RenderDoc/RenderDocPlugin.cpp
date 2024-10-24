#include "RenderDocPlugin.h"
#include "Core/AppCore.h"
#include "Controller/ControllerBase.h"
#include "Editor/Editor.h"
#include "Window/WindowsWindow.h"
#include "EAStdC/EADateTime.h"
#include "Utils/PathUtils.h"
#include "imgui.h"

static const AddPluginHelper<RenderDocPlugin> AddPlugin("RenderDocPlugin");

static int32_t NumFramesToCapture = 1;

eastl::string GetTimeString()
{
	time_t rawtime;
	struct tm* timeinfo = nullptr;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	EA::StdC::Strftime(buffer, sizeof(buffer), "%d-%m-%H-%M-%S", timeinfo);
	eastl::string str(buffer);

	return str;
}

void RenderDocPlugin::Init()
{
	return;

	PostInitCallback& postInitMulticast = GEngine->GetPostInitMulticast();
	postInitMulticast.BindRaw(this, &RenderDocPlugin::OnEngineInitDone);

	eastl::wstring renderDocPath;

	WindowsPlatform::QueryRegKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\", L"", renderDocPath);

	if (renderDocPath.length() == 0)
	{
		LOG_ERROR("RenderDoc plugin failed to initialized, could not find path.");
	}

	const size_t index = renderDocPath.find_last_of(L"\\");

	renderDocPath = renderDocPath.left(index + 1) + L"renderdoc.dll";

	mRenderDocDLL = LoadLibraryW(renderDocPath.c_str());

	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetRenderDocFunctionPointer(mRenderDocDLL, "RENDERDOC_GetAPI");

	const int32_t res = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void**)&RenderDocAPI);

	if (res == 0)
	{
		LOG_ERROR("RenderDoc plugin failed to initialized. Using incompatible version of RenderDoc.");
	}

	//Unregister crash handler unless the user has enabled it. This is to avoid sending unneccesary crash reports to Baldur.
	RenderDocAPI->UnloadCrashHandler();

	RenderDocAPI->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_CaptureCallstacks, 1);
	RenderDocAPI->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_CaptureAllCmdLists, 1);
	RenderDocAPI->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_SaveAllInitials, 1);
	//RenderDocAPI->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_DebugOutputMute, 0); // Not Working
	RenderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
	//RenderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_All, eRENDERDOC_Overlay_FrameRate);

	RenderDocAPI->SetFocusToggleKeys(nullptr, 0);
	RenderDocAPI->SetCaptureKeys(nullptr, 0);

	eastl::string directoryOutputPath = WindowsPlatform::GetExePathAnsi();
	directoryOutputPath += "\\RenderDocCaptures" ;
	eastl::string::const_iterator newEnd = eastl::remove_if(directoryOutputPath.begin(), directoryOutputPath.end(), isspace);

	// RenderDoc requires a path different than what Windows accepts(/ instead of \\)

	const bool dirExists = WindowsPlatform::DirectoryExistsInternal(directoryOutputPath);
	if (!dirExists)
	{
		bool directoryCreated = WindowsPlatform::CreateDirectoryTree(directoryOutputPath);
	}

	// Final member in the path is the file name for all RenderDoc files coming from this instance
	eastl::string capturesOutputPath = directoryOutputPath + "\\" + GetTimeString();
	PathUtils::NormalizeDirectory(capturesOutputPath);

	RenderDocAPI->SetLogFilePathTemplate(capturesOutputPath.c_str());
	//const char* logFilePath = RenderDocAPI->GetLogFilePathTemplate();

	bIsInit = true;
}


void RenderDocPlugin::OnCapturePressed()
{
	DoCapture();
}

void RenderDocPlugin::OnEngineInitDone()
{
	ControllerBase& editorController = GEditor->GetController();
	{
		KeyActionDelegate del = KeyActionDelegate::CreateRaw(this, &RenderDocPlugin::OnCapturePressed);
		EInputKey key = EInputKey::R;
		OnKeyAction action = { del, {}, key, true };
		editorController.AddListener(action);
	}
}

void RenderDocPlugin::Shutdown()
{
	FreeLibrary(mRenderDocDLL);
}

HWND GetWindowHandle()
{
	// or HWND handle = GetActiveWindow();
	HWND handle = static_cast<HWND>(GEngine->GetMainWindow().GetHandle());

	return handle;
}

void RenderDocPlugin::Tick(const float inDeltaTime)
{
	if (GEngine->IsImguiEnabled())
	{
		ImGui::Begin("RenderDoc"); // RHIWorkDisabled

		ImGui::SliderInt("Num Frames To Capture", &NumFramesToCapture, 1, 10);

		if (ImGui::Button("Do RenderDoc Capture")) // RHIWorkDisabled
		{
			DoCapture();
		}

		ImGui::End();
	}

	if (bCaptureInProgress)
	{
		if (GFrameCounter == CaptureEndFrame)
		{
			RenderDocAPI->EndFrameCapture(nullptr, GetWindowHandle());
			bCaptureInProgress = false;

			LaunchRenderDoc();
		}

		return;
	}

	if (bPendingCapture)
	{
		bPendingCapture = false;
		bCaptureInProgress = true;

		CaptureEndFrame = GFrameCounter + NumFramesToCapture;

		RenderDocAPI->StartFrameCapture(nullptr, GetWindowHandle());
	}

}

void RenderDocPlugin::DoCapture()
{
	bPendingCapture = true;
}

bool RenderDocPlugin::IsInit()
{
	return bIsInit;
}

void RenderDocPlugin::LaunchRenderDoc()
{
	char LogFile[512];
	uint64_t Timestamp;
	uint32_t LogPathLength = 512;
	uint32_t Index = 0;
	eastl::string LaunchPath;

	while (RenderDocAPI->GetCapture(Index, LogFile, &LogPathLength, &Timestamp))
	{
		LaunchPath = eastl::string(LogFile, LogPathLength);

		Index++;
	}

	PathUtils::NormalizeDirectory(LaunchPath);
	LaunchPath.insert(LaunchPath.begin(), '\"');
	LaunchPath.insert(LaunchPath.end() - 1, '\"');

	if (!LaunchPath.length() == 0)
	{
		if (!RenderDocAPI->IsRemoteAccessConnected())
		{
			uint32_t PID = RenderDocAPI->LaunchReplayUI(true, LaunchPath.c_str());
		}
	}
}

void* RenderDocPlugin::GetRenderDocFunctionPointer(HINSTANCE ModuleHandle, LPCSTR FunctionName)
{
	void* OutTarget = NULL;
	OutTarget = (void*)GetProcAddress(ModuleHandle, FunctionName);

	return OutTarget;
}
