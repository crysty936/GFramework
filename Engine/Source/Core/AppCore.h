#pragma once
#include "EASTL/vector.h"
#include "EASTL/shared_ptr.h"
#include "EventSystem/EventSystem.h"
#include "EASTL/string.h"
#include "InternalPlugins/IInternalPlugin.h"

using PostInitCallback = MulticastDelegate<>;

class AppCore
{
public:
	AppCore();
	~AppCore();

public:
	static void Init();
	static void Terminate();
	void Run();
	void CheckShouldCloseWindow();
	bool IsRunning();
	void StopEngine();
	bool IsImguiEnabled() const;

	class WindowsWindow& GetMainWindow() { return *MainWindow; }
	inline PostInitCallback& GetPostInitMulticast() { return InitDoneMulticast; }
	inline float GetCurrentDeltaT() { return CurrentDeltaT; }

	template<class PluginType>
	PluginType* GetInternalPlugin(const eastl::string& inName);

private:
	IInternalPlugin* GetPluginPrivate(const eastl::string& inName);

private:
	class AppModeBase* CurrentApp = nullptr;
	PostInitCallback InitDoneMulticast;
	float CurrentDeltaT;

	// TODO 
	// Engine core holds ownership over Window for now, it should be moved to application layer later
	eastl::unique_ptr<class WindowsWindow> MainWindow = nullptr;
};

extern AppCore* GEngine;
extern uint64_t GFrameCounter;

void AddInternalPlugin(class IInternalPlugin* inNewPlugin, const eastl::string& inName);

struct PluginAndName
{
	IInternalPlugin* Plugin = nullptr;
	eastl::string Name;
};

template<class T>
class AddPluginHelper
{
public:
	AddPluginHelper(const char* inPluginName)
	{
		AddInternalPlugin(new T(), inPluginName);
	}

	~AddPluginHelper() = default;
};


template<class PluginType>
PluginType* AppCore::GetInternalPlugin(const eastl::string& inName)
{
	IInternalPlugin* plugin = GetPluginPrivate(inName);

	if (plugin)
	{
		return static_cast<PluginType*>(plugin);
	}

	return nullptr;
}
