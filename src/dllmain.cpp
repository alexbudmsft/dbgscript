#include "common.h"
#include "../include/iscriptprovider.h"
#include <strsafe.h>
#include <assert.h>
#include "util.h"

static DllGlobals g_DllGlobals;

_Check_return_ DllGlobals*
GetDllGlobals()
{
	return &g_DllGlobals;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_DllGlobals.HModule = hinstDLL;
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

	hr = g_DllGlobals.DebugClient->QueryInterface(
		__uuidof(IDebugSystemObjects), (void **)&g_DllGlobals.DebugSysObj);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_DllGlobals.DebugClient->QueryInterface(
		__uuidof(IDebugSymbols3), (void **)&g_DllGlobals.DebugSymbols);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_DllGlobals.DebugClient->QueryInterface(
		__uuidof(IDebugAdvanced2), (void **)&g_DllGlobals.DebugAdvanced);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_DllGlobals.DebugClient->QueryInterface(
		__uuidof(IDebugDataSpaces4), (void **)&g_DllGlobals.DebugDataSpaces);
	if (FAILED(hr))
	{
		goto exit;
	}

	// TODO: Initialize all registered script providers.
	//

	// For now we only have one.
	//
	g_DllGlobals.ScriptProvider = CreateScriptProvider();
	if (!g_DllGlobals.ScriptProvider)
	{
		// Handle error.
		//
		hr = E_FAIL;
		goto exit;
	}

	hr = g_DllGlobals.ScriptProvider->Init();
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
	if (g_DllGlobals.ScriptProvider)
	{
		g_DllGlobals.ScriptProvider->Cleanup();
		g_DllGlobals.ScriptProvider = nullptr;
	}

	if (g_DllGlobals.DebugDataSpaces)
	{
		g_DllGlobals.DebugDataSpaces->Release();
		g_DllGlobals.DebugDataSpaces = nullptr;
	}

	if (g_DllGlobals.DebugAdvanced)
	{
		g_DllGlobals.DebugAdvanced->Release();
		g_DllGlobals.DebugAdvanced = nullptr;
	}

	if (g_DllGlobals.DebugSymbols)
	{
		g_DllGlobals.DebugSymbols->Release();
		g_DllGlobals.DebugSymbols = nullptr;
	}

	if (g_DllGlobals.DebugSysObj)
	{
		g_DllGlobals.DebugSysObj->Release();
		g_DllGlobals.DebugSysObj = nullptr;
	}

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

// comma-separated list of paths.
// If run with no parameters, print the current path.
//
DLLEXPORT HRESULT CALLBACK
scriptpath(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         args)
{
	// Deallocate the existing list.
	//
	ScriptPathElem* elem = nullptr;
	ScriptPathElem* prev = nullptr;
	const char* comma = nullptr;
	const char* str = args;
	HRESULT hr = S_OK;

	if (strlen(str) > 0)
	{
		elem = g_DllGlobals.ScriptPath;
		while (elem)
		{
			ScriptPathElem* next = elem->Next;
			delete elem;
			elem = next;
		}

		elem = g_DllGlobals.ScriptPath = nullptr;

		// Can't use semi-colon since the debugger's command parser interprets that
		// as a new command. Use comma instead.
		//
		comma = strchr(str, ',');
		while (comma)
		{
			int len = (int)(comma - str);

			elem = new ScriptPathElem;
			hr = StringCchCopyNA(STRING_AND_CCH(elem->Path), str, len);
			assert(SUCCEEDED(hr));

			if (!g_DllGlobals.ScriptPath)
			{
				// Save the head.
				//
				g_DllGlobals.ScriptPath = elem;
				assert(!prev);
			}
			else
			{
				prev->Next = elem;
			}

			prev = elem;

			str += len + 1;
			comma = strchr(str, ',');
		}

		// Copy trailer.
		//
		elem = new ScriptPathElem;
		StringCchCopyA(STRING_AND_CCH(elem->Path), str);
		assert(SUCCEEDED(hr));

		if (!g_DllGlobals.ScriptPath)
		{
			// Save the head.
			//
			g_DllGlobals.ScriptPath = elem;
			assert(!prev);
		}
		else
		{
			prev->Next = elem;
		}
	}

	// Print current path.
	//
	elem = g_DllGlobals.ScriptPath;
	while (elem)
	{
		hr = GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_NORMAL,
			"Script path: '%s'\n", elem->Path);
		if (FAILED(hr))
		{
			goto exit;
		}
		elem = elem->Next;
	}
exit:
	return hr;
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
DLLEXPORT HRESULT CALLBACK
runscript(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         args)
{
	HRESULT hr = S_OK;
	int cArgs = 0;
	WCHAR** argList = nullptr;
	const WCHAR* wszArgs = nullptr;

	if (!args[0])
	{
		// If it's an empty string, fail now.
		//
		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: !runscript requires at least one argument.\n");
		hr = E_INVALIDARG;
		goto exit;
	}

	wszArgs = UtilConvertAnsiToWide(args);
	if (!wszArgs)
	{
		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to convert args to wide string.\n");
		hr = E_FAIL;
		goto exit;
	}

	argList = CommandLineToArgvW(wszArgs, &cArgs);
	if (!argList)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to parse arguments: 0x%08x\n", hr);
		goto exit;
	}


	// In the future, !runscript will keep the first several args for itself,
	// and pass the rest on to the script provider.
	//
	// Currently, it doesn't so we just pass everything.
	//
	// Rebase the arguments from the script to after what we consumed for the
	// host.
	//
	WCHAR** argsForScriptProv = &argList[0];
	int cArgsForScriptProv = cArgs;
	assert(cArgsForScriptProv >= 0);

	// We should examine the extension of the script and walk the list
	// of registered providers to find the first one that claims the extension.
	// For now, we only have one provider.
	//
	hr = GetDllGlobals()->ScriptProvider->Run(cArgsForScriptProv, argsForScriptProv);
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	if (FAILED(hr))
	{
		GetDllGlobals()->DebugControl->Output(DEBUG_OUTPUT_ERROR, "Script failed: 0x%08x.\n", hr);
	}
	if (wszArgs)
	{
		delete[] wszArgs;
		wszArgs = nullptr;
	}
	if (argList)
	{
		LocalFree(argList);
		argList = nullptr;
	}
	return hr;
}

// Runs a script in a string. Currently this is hardcoded to python, but we'll
// extend with a "language" parameter once we have other providers.
//
DLLEXPORT HRESULT CALLBACK
evalstring(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         args)
{
	HRESULT hr = S_OK;

	// We should examine the extension of the script and walk the list
	// of registered providers to find the first one that claims the extension.
	// For now, we only have one provider.
	//
	hr = GetDllGlobals()->ScriptProvider->RunString(args);
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	if (FAILED(hr))
	{
		GetDllGlobals()->DebugControl->Output(DEBUG_OUTPUT_ERROR, "Script failed: 0x%08x.\n", hr);
	}
	return hr;
}



