#pragma once

#include <hostcontext.h>

struct RubyProvGlobals
{
	HMODULE HModule;
	DbgScriptHostContext* HostCtxt;
};

_Check_return_ RubyProvGlobals*
GetRubyProvGlobals();

// Helper macro to call in every Python entry point that checks for abort
// and raises a KeyboardInterrupt exception.
//
#define CHECK_ABORT(ctxt) \
	do { \
		if (UtilCheckAbort(ctxt)) \
		{ \
			rb_raise(rb_eInterrupt, "Execution interrupted."); \
		} \
	} while (0)
