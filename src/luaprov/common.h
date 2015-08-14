#pragma once

#include <hostcontext.h>
#include "../common.h"
#include "../support/util.h"

struct LuaProvGlobals
{
	HMODULE HModule;
	DbgScriptHostContext* HostCtxt;

	// Input buffer for getc support.
	//
	char InputBuf[2048];

	// Next input char to be returned from getc callback.
	//
	int NextInputChar;

	ULONG ValidInputChars;
};

_Check_return_ LuaProvGlobals*
GetLuaProvGlobals();
