#include "Core/AppModeBase.h"
#include "EngineUtils.h"

AppModeBase* AppModeBase::GameMode = new AppModeBase();

AppModeBase::AppModeBase()
{
	ASSERT(!GameMode);

	GameMode = this;
}

AppModeBase::~AppModeBase()
= default;

void AppModeBase::BeginFrame()
{

}

void AppModeBase::Draw()
{

}

void AppModeBase::EndFrame()
{

}

void AppModeBase::Init()
{











}

void AppModeBase::Terminate()
{
	ASSERT(GameMode);

	delete GameMode;
	GameMode = nullptr;
}

void AppModeBase::Tick(float inDeltaT)
{













}
