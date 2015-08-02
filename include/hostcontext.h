#pragma once

#include <windows.h>
#include <dbgeng.h>

struct IScriptProvider;
class DbgScriptOutputCallbacks;

struct ScriptPathElem
{
	ScriptPathElem() :
		Next(nullptr)
	{}

	ScriptPathElem* Next;

	char Path[MAX_PATH];
};

struct DbgScriptHostContext
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

	DbgScriptOutputCallbacks* BufferedOutputCallbacks;
};

char*
DbgScriptOutCallbacksGetBuffer(
	_In_ DbgScriptOutputCallbacks* cb);