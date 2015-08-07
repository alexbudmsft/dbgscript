#pragma once

#pragma warning(push)
#pragma warning(disable: 4510 4512 4610 4100)
#include <ruby.h>
#pragma warning(pop)

#include <hostcontext.h>
#include "../support/util.h"
#include "../support/symcache.h"
#include "util.h"

struct RubyProvGlobals
{
	// Ruby Provider DLL Module handle.
	//
	HMODULE HModule;

	// DbgScript Host context block.
	//
	DbgScriptHostContext* HostCtxt;

	// DbgScript Ruby Module.
	//
	VALUE DbgScriptModule;

	// Ruby DbgScript::Thread class.
	//
	VALUE ThreadClass;

	// Ruby DbgScript::StackFrame class.
	//
	VALUE StackFrameClass;
	
	// Ruby DbgScript::TypedObject class.
	//
	VALUE TypedObjectClass;
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
