#pragma once
#include "Logger/Logger.h"
#include <stdlib.h>
#include <iostream>

//#ifndef NDEBUG

// We use the basic assumption that if x is true, then there's no need to validate the second condition

#define ASSERT_MSG(x, inMessage, ...)						\
  ((!!(x)) || ([&](){										\
LOG_ERROR(inMessage, __VA_ARGS__);							\
 __debugbreak();											\
 return false;												\
  }()))		

#define ENSURE_MSG(x, inMessage, ...)						\
  ((!!(x)) || ([&](){										\
static bool bExecuted = false;								\
  if(!bExecuted)											\
  {															\
bExecuted = true;											\
LOG_ERROR(inMessage, __VA_ARGS__);							\
 __debugbreak();}											\
 return false;												\
  }()))														\

#define ASSERT(x, ...)										\
  ((!!(x)) || ([&](){										\
 __debugbreak();											\
 return false;												\
  }()))														\

#define ENSURE(x)											\
  ((!!(x)) || ([](){										\
static bool bExecuted = false;								\
  if(!bExecuted)											\
  {															\
bExecuted = true;											\
 __debugbreak();}											\
 return false;												\
  }()))														\

//#else
//#define ASSERT_MSG(x, ...)	(!!(x))
//#define ASSERT(x, ...)		(!!(x))
//#define ENSURE_MSG(x, ...)	(!!(x))
//#define ENSURE(x)			(!!(x))
//#endif



#define OUT