#pragma once

class IInternalPlugin
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	virtual void Tick(const float inDeltaTime) = 0;

	virtual bool IsInit() = 0;
};