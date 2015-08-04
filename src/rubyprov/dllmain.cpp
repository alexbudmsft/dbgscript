#include <windows.h>
#include <iscriptprovider.h>
#include <hostcontext.h>
#include "common.h"

static RubyProvGlobals s_RubyProvGlobals;

_Check_return_ RubyProvGlobals*
GetRubyProvGlobals()
{
	return &s_RubyProvGlobals;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		s_RubyProvGlobals.HModule = hinstDLL;
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;  // Nothing to do.
	default:
		break;
	}

	return TRUE;
}
