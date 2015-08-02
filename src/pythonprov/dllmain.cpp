#include "windows.h"
#include <iscriptprovider.h>
#include <hostcontext.h>
#include "common.h"

static PythonProvGlobals s_PythonProvGlobals;

_Check_return_ PythonProvGlobals*
GetPythonProvGlobals()
{
	return &s_PythonProvGlobals;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		s_PythonProvGlobals.HModule = hinstDLL;
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
