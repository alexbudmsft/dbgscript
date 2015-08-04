#pragma once

#include <windows.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

// Export names for all providers.
//
#define SCRIPT_PROV_INIT	"ScriptProviderInit"
#define SCRIPT_PROV_CLEANUP "ScriptProviderCleanup"
#define SCRIPT_PROV_CREATE "ScriptProviderCreate"

struct DbgScriptHostContext;

// IScriptProvider - This interface must be implemented by all script providers.
//
struct IScriptProvider
{
	virtual _Check_return_ HRESULT 
	Init() = 0;

	virtual _Check_return_ HRESULT 
	Run(
		_In_ int argc,
		_In_ WCHAR** argv) = 0;

	virtual _Check_return_ HRESULT
	RunString(
		_In_z_ const char* scriptString) = 0;

	virtual _Check_return_ void
	Cleanup() = 0;
};

typedef HRESULT
(*SCRIPT_PROV_INIT_FUNC)(
	_In_ DbgScriptHostContext*);

typedef void
(*SCRIPT_PROV_CLEANUP_FUNC) ();

// Factory for IScriptProviders.
//
typedef IScriptProvider*
(*SCRIPT_PROV_CREATE_FUNC) ();
