#pragma once

#include <windows.h>
#include <dbgeng.h>

#define DLLEXPORT extern "C" __declspec(dllexport)
#define STRING_AND_CCH(x) x, _countof(x)

// Limits of our extension.
//
const int MAX_MODULE_NAME_LEN = 256;
const int MAX_SYMBOL_NAME_LEN = 512;

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
	IDebugAdvanced2* DebugAdvanced;
	IDebugDataSpaces4* DebugDataSpaces;

	// FUTURE: This will be a list of all script providers.
	//
	IScriptProvider* ScriptProvider;

	ScriptPathElem* ScriptPath;

	// Are we buffering output? This is a refcount to support nested calls.
	//
	LONG IsBuffering;

	// Buffer used for output if buffering enabled.
	//
	char MessageBuf[4096];

	size_t BufPosition;
};

_Check_return_ DllGlobals*
GetDllGlobals();
