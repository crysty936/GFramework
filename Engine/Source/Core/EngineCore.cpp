#include "Core/EngineCore.h"
#include "Core/WindowsPlatform.h"
#include "Logger/Logger.h"
#include "InputSystem/InputSystem.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Core/GameModeBase.h"
#include "Timer/TimersManager.h"
#include "Renderer/Material/MaterialsManager.h"
#include "Window/WindowsWindow.h"
#include "Renderer/RHI/RHI.h"
#include "Renderer/RHI/D3D12/D3D12RHI.h"
#include "Renderer/Renderer.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include "Editor/Editor.h"
#include "InternalPlugins/IInternalPlugin.h"

constexpr float IdealFrameRate = 60.f;
constexpr float IdealFrameTime = 1.0f / IdealFrameRate;
bool bIsRunning = true;

EngineCore* GEngine = nullptr;
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

EngineCore::EngineCore()
	: CurrentDeltaT{ 0.f }
{
	static bool engineExists = false;
	ASSERT(!engineExists);

	engineExists = true;
}

EngineCore::~EngineCore() = default;

// Init all engine subsystems
void EngineCore::Init()
{
	GEngine = new EngineCore{};
	GEngine->CurrentGameMode = GameModeBase::Get();

	InputSystem::Init();

	// Create Main Window
	GEngine->MainWindow = eastl::make_unique<WindowsWindow>();

	SceneManager::Init();

	MaterialsManager::Init();

	 //Initialize plugins before renderer to make sure that RenderDoc works
 	for (PluginAndName& container : GetInternalPluginsList())
 	{
 		container.Plugin->Init();
 	}

	D3D12RHI::Init();
	D3D12RHI::Get()->ImGuiInit();

	 //Hide Cursor for input
	//InputSystem::Get().SetCursorMode(ECursorMode::Disabled, GEngine->MainWindow->GetHandle());// RHIWorkDisabled

	Renderer::Init();// RHIWorkDisabled

	TimersManager::Init();

	SceneManager::Get().LoadScene();

	 //TODO [Editor-Game Separation]: Only if compiled with editor
	Editor::Init();

	 //After initializing all engine subsystems, Game Mode init is called 
	 //TODO [Editor-Game Separation]: This should be initialized like this only when editor is missing otherwise by the editor
	SceneManager::Get().GetCurrentScene().InitObjects();

	GEngine->InitDoneMulticast.Invoke();
}


void EngineCore::Terminate()
{

	// TODO [Editor-Game Separation]: Only if compiled with editor
	Editor::Terminate();

	TimersManager::Terminate();
	Renderer::Terminate();

	for (PluginAndName& container : GetInternalPluginsList())
	{
		container.Plugin->Shutdown();
	}

	InputSystem::Terminate();

	D3D12RHI::Terminate();
	MaterialsManager::Terminate();

	SceneManager::Terminate();

	ASSERT(GEngine);
	delete GEngine;
}

void EngineCore::Run()
{
	WindowsPlatform::InitCycles();
	double deltaTime = 0.0;
	double lastTime = WindowsPlatform::GetTime();

	while (bIsRunning)
	{
		double currentTime = WindowsPlatform::GetTime();
		double timeSpent = currentTime - lastTime;
		double timeLeft = IdealFrameTime - timeSpent;

		// Sleep 0 until time is out, granularity can be set to avoid this but this works well
		while (timeLeft > 0)
		{
			WindowsPlatform::Sleep(0);

			currentTime = WindowsPlatform::GetTime();
			timeSpent = currentTime - lastTime;

			timeLeft = IdealFrameTime - timeSpent;
		}

		CurrentDeltaT = static_cast<float>(currentTime - lastTime);
		eastl::wstring text;
		text.sprintf(L"Seconds: %f", CurrentDeltaT);
		WindowsPlatform::SetWindowsWindowText(text);

		//Logger::GetLogger().Log("Delta time: %lf", CurrentDeltaT);
		lastTime = currentTime;

		InputSystem::Get().PollEvents();

		//Call tickableObjects (Camera, etc)

		 //Tick Timers
		TimersManager::Get().TickTimers(CurrentDeltaT);

		D3D12RHI::Get()->ImGuiBeginFrame();
 		ImGui_ImplWin32_NewFrame();
 		ImGui::NewFrame();
 
 		//ImGui::ShowDemoWindow();

 		//SceneManager::Get().GetCurrentScene().TickObjects(CurrentDeltaT);// RHIWorkDisabled
 		//SceneManager::Get().GetCurrentScene().DisplayObjects();// RHIWorkDisabled

		 //TODO [Editor-Game Separation]: If editor is not present
 		//CurrentGameMode->Tick(CurrentDeltaT); 

		D3D12RHI::Get()->BeginFrame();

		GEditor->Tick(CurrentDeltaT);

		//Renderer::Get().Draw();// RHIWorkDisabled
		D3D12RHI::Get()->Test();

 		for (PluginAndName& container : GetInternalPluginsList())
 		{
 			container.Plugin->Tick(static_cast<float>(deltaTime));
 		}

		 //Draw ImGui
		ImGui::EndFrame();
		ImGui::Render();
		D3D12RHI::Get()->ImGuiRenderDrawData();

		D3D12RHI::Get()->SwapBuffers(); //Renderer::Get().Present();// RHIWorkDisabled

		CheckShouldCloseWindow();

		++GFrameCounter;
	}

	Terminate();
}

void EngineCore::CheckShouldCloseWindow()
{
	if (MainWindow->ShouldClose())
	{
		StopEngine();
	}
}

bool EngineCore::IsRunning()
{
	return bIsRunning;
}

void EngineCore::StopEngine()
{
	bIsRunning = false;
}

IInternalPlugin* EngineCore::GetPluginPrivate(const eastl::string& inName)
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
