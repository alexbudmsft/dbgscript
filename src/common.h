#pragma once

#include <windows.h>
#include <dbgeng.h>

#define DLLEXPORT extern "C" __declspec(dllexport)
#define STRING_AND_CCH(x) x, _countof(x)

struct IScriptProvider;

struct ScriptPathElem
{
	ScriptPathElem() :
		Next(nullptr)
	{}

	ScriptPathElem* Next;

	char Path[MAX_PATH];
};

struct DllGlobals
{
	HINSTANCE HModule;

	// DbgEng Interfaces.
	//
	IDebugClient* DebugClient;
	IDebugControl* DebugControl;
	IDebugSystemObjects* DebugSysObj;
	IDebugSymbols3* DebugSymbols;

	// FUTURE: This will be a list of all script providers.
	//
	IScriptProvider* ScriptProvider;

	ScriptPathElem* ScriptPath;
};

_Check_return_ DllGlobals*
GetDllGlobals();
