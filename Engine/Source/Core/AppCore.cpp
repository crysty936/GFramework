#include "Core/AppCore.h"
#include "Core/WindowsPlatform.h"
#include "Logger/Logger.h"
#include "InputSystem/InputSystem.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Core/AppModeBase.h"
#include "Timer/TimersManager.h"
#include "Window/WindowsWindow.h"
#include "Editor/Editor.h"
#include "InternalPlugins/IInternalPlugin.h"
#include "imgui_internal.h"
#include "Utils/PerfUtils.h"

constexpr float IdealFrameRate = 60.f;
constexpr float IdealFrameTime = 1.0f / IdealFrameRate;
bool bIsRunning = true;

AppCore* GEngine = nullptr;
uint64_t GFrameCounter = 0;

static eastl::vector<PluginAndName>& GetInternalPluginsList()
{
	static eastl::vector<PluginAndName> InternalPluginsList;

	return InternalPluginsList;
}

void AddInternalPlugin(IInternalPlugin* inNewPlugin, const eastl::string& inName)
{
	GetInternalPluginsList().push_back({ inNewPlugin, inName });
}

AppCore::AppCore()
	: CurrentDeltaT{ 0.f }
{
	static bool engineExists = false;
	ASSERT(!engineExists);

	engineExists = true;
}

AppCore::~AppCore() = default;

// Init all engine subsystems
void AppCore::Init()
{
	BENCH_SCOPE("Engine Init");

	GEngine = new AppCore{};

	InputSystem::Init();

	// Create Main Window
	GEngine->MainWindow = eastl::make_unique<WindowsWindow>();

	SceneManager::Init();

	 //Initialize plugins before renderer to make sure that RenderDoc works
 	for (PluginAndName& container : GetInternalPluginsList())
 	{
 		container.Plugin->Init();
 	}

	TimersManager::Init();
	Editor::Init();

	GEngine->CurrentApp = AppModeBase::Get();
	GEngine->CurrentApp->Init();

	 //Hide Cursor for input
	//InputSystem::Get().SetCursorMode(ECursorMode::Disabled, GEngine->MainWindow->GetHandle());// RHIWorkDisabled

	 //After initializing all engine subsystems, Game Mode init is called 
	 //TODO [Editor-Game Separation]: This should be initialized like this only when editor is missing otherwise by the editor
	SceneManager::Get().GetCurrentScene().InitObjects();

	GEngine->InitDoneMulticast.Invoke();
}


void AppCore::Terminate()
{
	Editor::Terminate();

	TimersManager::Terminate();

	for (PluginAndName& container : GetInternalPluginsList())
	{
		container.Plugin->Shutdown();
	}

	GEngine->CurrentApp->Terminate();

	InputSystem::Terminate();

	SceneManager::Terminate();

	ASSERT(GEngine);
	delete GEngine;
}

static inline float CalculateDeltaTAndWait(double& deltaTime, double& lastTime)
{
	double currentTime = WindowsPlatform::GetTime();
	double timeSpent = currentTime - lastTime;
	double timeLeft = IdealFrameTime - timeSpent;

	// Sleep 0 until time is out, granularity can be set to avoid this but it works well
	while (timeLeft > 0)
	{
		WindowsPlatform::Sleep(0);

		currentTime = WindowsPlatform::GetTime();
		timeSpent = currentTime - lastTime;

		timeLeft = IdealFrameTime - timeSpent;
	}

	const float currentDeltaT = static_cast<float>(currentTime - lastTime);
	//Logger::GetLogger().Log("Delta time: %lf", currentDeltaT);
	lastTime = currentTime;

	return currentDeltaT;
}

void AppCore::Run()
{
	WindowsPlatform::InitCycles();
	double deltaTime = 0.0;
	double lastTime = WindowsPlatform::GetTime();

	while (bIsRunning)
	{
		const float CurrentDeltaT = CalculateDeltaTAndWait(deltaTime, lastTime);

		eastl::wstring text;
		text.sprintf(L"Seconds: %f", CurrentDeltaT);
		WindowsPlatform::SetWindowsWindowText(text);

		InputSystem::Get().PollEvents();

		 //Tick Timers
		TimersManager::Get().TickTimers(CurrentDeltaT);

		CurrentApp->BeginFrame();

		GEditor->Tick(CurrentDeltaT);

		CurrentApp->Tick(CurrentDeltaT);
		CurrentApp->Draw();

		// Tick plugins
 		for (PluginAndName& container : GetInternalPluginsList())
 		{
			if (!container.Plugin->IsInit())
			{
				continue;
			}

 			container.Plugin->Tick(static_cast<float>(deltaTime));
 		}

		CurrentApp->EndFrame();

		CheckShouldCloseWindow();

		++GFrameCounter;
	}

	Terminate();
}

void AppCore::CheckShouldCloseWindow()
{
	if (MainWindow->ShouldClose())
	{
		StopEngine();
	}
}

bool AppCore::IsRunning()
{
	return bIsRunning;
}

void AppCore::StopEngine()
{
	bIsRunning = false;
}

bool AppCore::IsImguiEnabled() const
{
	return !!GImGui;
}

IInternalPlugin* AppCore::GetPluginPrivate(const eastl::string& inName)
{
	for (PluginAndName& container : GetInternalPluginsList())
	{
		if (container.Name == inName)
		{
			return container.Plugin;
		}
	}

	return nullptr;
}
