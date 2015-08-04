#pragma once

#include <windows.h>
#include <dbgeng.h>

struct IScriptProvider;
struct ScriptProviderInfo;
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
	// Handle to the DbgScript DLL.
	//
	HINSTANCE HModule;

	// DbgEng Interfaces.
	//
	IDebugClient* DebugClient;
	IDebugControl* DebugControl;
	IDebugSystemObjects* DebugSysObj;
	IDebugSymbols3* DebugSymbols;
	IDebugAdvanced2* DebugAdvanced;
	IDebugDataSpaces4* DebugDataSpaces;

	// List of all registered script providers (based on registry.)
	//
	ScriptProviderInfo* ScriptProviders;

	// Script path to search for scripts (!scriptpath)
	//
	ScriptPathElem* ScriptPath;

	// Are we buffering output? This is a refcount to support nested calls.
	//
	LONG IsBuffering;

	// Buffer used for output if buffering enabled.
	//
	char MessageBuf[8192];

	// Position in 'MessageBuf'.
	//
	size_t BufPosition;

	// BufferedOutputCallbacks - Output callback for DbgEng to capture and
	// buffer output.
	//
	DbgScriptOutputCallbacks* BufferedOutputCallbacks;
};

char*
DbgScriptOutCallbacksGetBuffer(
	_In_ DbgScriptOutputCallbacks* cb);