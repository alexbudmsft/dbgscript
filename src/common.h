#pragma once

#include <windows.h>
#include <dbgeng.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

struct DllGlobals
{
	HINSTANCE HModule;
	IDebugClient* DebugClient;
	IDebugControl* DebugControl;
};


_Check_return_ DllGlobals*
GetDllGlobals();
