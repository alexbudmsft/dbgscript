#pragma once

#include <hostcontext.h>

struct PythonProvGlobals
{
	HMODULE HModule;
	DbgScriptHostContext* HostCtxt;
};

_Check_return_ PythonProvGlobals*
GetPythonProvGlobals();
