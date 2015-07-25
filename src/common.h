#pragma once

#include <windows.h>
#include <dbgeng.h>

#define DLLEXPORT extern "C" __declspec(dllexport)
struct IScriptProvider;

struct DllGlobals
{
	HINSTANCE HModule;
	IDebugClient* DebugClient;
	IDebugControl* DebugControl;

	// FUTURE: This will be a list of all script providers.
	//
	IScriptProvider* ScriptProvider;
};


_Check_return_ DllGlobals*
GetDllGlobals();
