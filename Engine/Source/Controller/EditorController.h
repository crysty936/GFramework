#pragma once
#include "ControllerBase.h"

class EditorController : public ControllerBase
{
public:
	EditorController();
	virtual ~EditorController();

	virtual void ExecuteCallbacks() override;
};