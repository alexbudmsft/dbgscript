#pragma once

#include <hostcontext.h>
#include "../common.h"
#include "../support/util.h"

struct LuaProvGlobals
{
	HMODULE HModule;
	DbgScriptHostContext* HostCtxt;
};

_Check_return_ LuaProvGlobals*
GetLuaProvGlobals();
