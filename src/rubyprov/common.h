#pragma once

#include <hostcontext.h>

struct RubyProvGlobals
{
	HMODULE HModule;
	DbgScriptHostContext* HostCtxt;
};

_Check_return_ RubyProvGlobals*
GetRubyProvGlobals();
