#include "EABase/eabase.h"

// Required overloads of operator new for EASTL

void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

void* __cdecl operator new[](unsigned __int64 size, unsigned __int64, unsigned __int64, char const*, int, unsigned int, char const*, int)
{
	return new uint8_t[size];
}