#include "common.h"
#include "../include/iscriptprovider.h"

static DllGlobals g_DllGlobals;

_Check_return_ DllGlobals*
GetDllGlobals()
{
	return &g_DllGlobals;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE /* hinstDLL */,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;  // Nothing to do.
	default:
		break;
	}

	return TRUE;
}

// Called to initialize the DLL.
//
DLLEXPORT HRESULT DebugExtensionInitialize(
	_Out_ PULONG Version,
	_Out_ PULONG Flags)
{
	HRESULT hr = S_OK;
	*Version = DEBUG_EXTENSION_VERSION(1, 0);
	*Flags = 0;

	hr = DebugCreate(__uuidof(IDebugClient), (void **)&g_DllGlobals.DebugClient);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_DllGlobals.DebugClient->QueryInterface(
		__uuidof(IDebugControl), (void **)&g_DllGlobals.DebugControl);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
	// Here all the registered engines should be initialized.
	//
	return hr;
}

// Called to uninitialize the DLL.
//
DLLEXPORT void CALLBACK 
DebugExtensionUninitialize()
{
	if (g_DllGlobals.DebugControl)
	{
		g_DllGlobals.DebugControl->Release();
		g_DllGlobals.DebugControl = nullptr;
	}

	if (g_DllGlobals.DebugClient)
	{
		g_DllGlobals.DebugClient->Release();
		g_DllGlobals.DebugClient = nullptr;
	}
}

// TODO: Use script provider based on file extension.
// Later, allow ini file for registration of script provider DLLs with mapping from extension to DLL.
// Or maybe have a callback in each DLL that says which file extensions it supports.
//
// Debugger syntax: !runscript <scriptfile>
//
// 'args' is a single string with the entire command line passed in, unprocessed.
// E.g. if run with !runscript a b c d blah
// then args will be "a b c d blah"
//
// We have to do the tokenization ourselves.
//
DLLEXPORT HRESULT CALLBACK runscript(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         args)
{
	//IDebugControl *dbgCtrl = nullptr;
	//HRESULT hr = client->QueryInterface(__uuidof(IDebugControl), (void**)&dbgCtrl);
	HRESULT hr = S_OK;
	IScriptProvider* prov = nullptr;

	hr = GetDllGlobals()->DebugControl->Output(DEBUG_OUTPUT_NORMAL, "%s\n", args);
	if (FAILED(hr))
	{
		goto exit;
	}

	const char* scriptName = args;

	scriptName; // TODO convert to wide string.

	// We should examine the extension of the script and walk the list
	// of registered providers to find the first one that claims the extension.
	//

	prov = CreateScriptProvider(L"aaa");
	if (!prov)
	{
		// Handle error.
		//
		hr = E_FAIL;
		goto exit;
	}

	hr = prov->Init();
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
	if (prov)
	{
		prov->Cleanup();
		prov = nullptr;
	}

	return hr;
}


