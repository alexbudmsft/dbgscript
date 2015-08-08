#include "common.h"

static RubyProvGlobals s_RubyProvGlobals;

_Check_return_ RubyProvGlobals*
GetRubyProvGlobals()
{
	return &s_RubyProvGlobals;
}

// Don't define a DllMain as ruby static lib already does.
// Fine. Let them have it.
//
