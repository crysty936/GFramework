#pragma once

enum class EInputType
{
	None,
  	InputPress,
	InputRelease,
	InputRepeat
};

const char* ToString(EInputType inEvent);