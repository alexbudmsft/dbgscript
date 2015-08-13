#include "common.h"

static LuaProvGlobals s_LuaProvGlobals;

_Check_return_ LuaProvGlobals*
GetLuaProvGlobals()
{
	return &s_LuaProvGlobals;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		s_LuaProvGlobals.HModule = hinstDLL;
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
