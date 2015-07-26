#pragma once

#include <windows.h>
#include <dbgeng.h>

#define DLLEXPORT extern "C" __declspec(dllexport)
#define STRING_AND_CCH(x) x, _countof(x)

struct IScriptProvider;

struct DllGlobals
{
	HINSTANCE HModule;
	IDebugClient* DebugClient;
	IDebugControl* DebugControl;
	IDebugSystemObjects* DebugSysObj;
	IDebugSymbols3* DebugSymbols;

	// FUTURE: This will be a list of all script providers.
	//
	IScriptProvider* ScriptProvider;
};


_Check_return_ DllGlobals*
GetDllGlobals();
