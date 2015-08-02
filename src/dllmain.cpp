#include "common.h"
#include <strsafe.h>
#include <assert.h>
#include "support/util.h"

static DbgScriptHostContext g_HostCtxt;

_Check_return_ DbgScriptOutputCallbacks*
GetDbgScriptOutputCb();

_Check_return_ DbgScriptHostContext*
GetHostContext()
{
	return &g_HostCtxt;
}

struct ScriptProviderCallbacks
{
	// Module handle.
	//
	HMODULE Module;

	// DLL initialization.
	//
	SCRIPT_PROV_INIT_FUNC InitFunc;

	// DLL cleanup.
	//
	SCRIPT_PROV_CLEANUP_FUNC CleanupFunc;

	// Factory function.
	//
	SCRIPT_PROV_CREATE_FUNC CreateFunc;
};

struct ScriptProvCallbackBinding
{
	const char* ExportName;

	int CallbackOffset;
};

// Functions to be bound with their offsets in the structure.
//
static const ScriptProvCallbackBinding x_CallbackBindings[] =
{
	{ SCRIPT_PROV_INIT, offsetof(ScriptProviderCallbacks, InitFunc) },
	{ SCRIPT_PROV_CLEANUP, offsetof(ScriptProviderCallbacks, CleanupFunc) },
	{ SCRIPT_PROV_CREATE, offsetof(ScriptProviderCallbacks, CreateFunc) },
};

// TODO: For now, hardcode a single provider. Later this will be a list.
//
static ScriptProviderCallbacks s_PyProvCb;

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_HostCtxt.HModule = hinstDLL;
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

	hr = DebugCreate(__uuidof(IDebugClient), (void **)&g_HostCtxt.DebugClient);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_HostCtxt.DebugClient->QueryInterface(
		__uuidof(IDebugControl), (void **)&g_HostCtxt.DebugControl);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_HostCtxt.DebugClient->QueryInterface(
		__uuidof(IDebugSystemObjects), (void **)&g_HostCtxt.DebugSysObj);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_HostCtxt.DebugClient->QueryInterface(
		__uuidof(IDebugSymbols3), (void **)&g_HostCtxt.DebugSymbols);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_HostCtxt.DebugClient->QueryInterface(
		__uuidof(IDebugAdvanced2), (void **)&g_HostCtxt.DebugAdvanced);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = g_HostCtxt.DebugClient->QueryInterface(
		__uuidof(IDebugDataSpaces4), (void **)&g_HostCtxt.DebugDataSpaces);
	if (FAILED(hr))
	{
		goto exit;
	}

	g_HostCtxt.BufferedOutputCallbacks = GetDbgScriptOutputCb();

	// TODO: Initialize all registered script providers.
	//

	// TODO: Expose an ini file with pointers to various providers for registration.
	//
	WCHAR dllPath[MAX_PATH] = {};
	WCHAR pythonProv[MAX_PATH] = {};
	WCHAR pythonProvDll[MAX_PATH] = {};
	GetModuleFileName(g_HostCtxt.HModule, dllPath, _countof(dllPath));
	WCHAR* lastBackSlash = wcsrchr(dllPath, L'\\');
	assert(lastBackSlash);
	// Null out the slash to get only the directory path of the DLL.
	//
	*lastBackSlash = 0;
	StringCchPrintf(STRING_AND_CCH(pythonProv), L"%s\\pythonprov", dllPath);
	StringCchPrintf(STRING_AND_CCH(pythonProvDll), L"%s\\pythonprov.dll", pythonProv);

	BOOL fOk = SetDllDirectory(pythonProv);
	assert(fOk);

	s_PyProvCb.Module = LoadLibrary(pythonProvDll);

	// TODO: Real error handling.
	//
	assert(s_PyProvCb.Module);

	// Bind all the callbacks.
	//
	for (int i = 0; i < _countof(x_CallbackBindings); ++i)
	{
		DWORD_PTR* storeLoc = (DWORD_PTR*)((BYTE*)&s_PyProvCb + x_CallbackBindings[i].CallbackOffset);
		*storeLoc = (DWORD_PTR)GetProcAddress(s_PyProvCb.Module, x_CallbackBindings[i].ExportName);

		// TODO: Real error handling.
		//
		assert(*storeLoc);
	}

	// Initialize the provider DLL.
	//
	s_PyProvCb.InitFunc(&g_HostCtxt);

	// For now we only have one.
	//
	g_HostCtxt.ScriptProvider = s_PyProvCb.CreateFunc();
	if (!g_HostCtxt.ScriptProvider)
	{
		// Handle error.
		//
		hr = E_FAIL;
		goto exit;
	}

	hr = g_HostCtxt.ScriptProvider->Init();
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
	// Call the provider instance's cleanup routine.
	//
	if (g_HostCtxt.ScriptProvider)
	{
		g_HostCtxt.ScriptProvider->Cleanup();
		g_HostCtxt.ScriptProvider = nullptr;
	}

	// Call the provider's DLL cleanup routine.
	//
	s_PyProvCb.CleanupFunc();

	// Unload the provider DLL.
	//
	BOOL fOk = FreeLibrary(s_PyProvCb.Module);
	assert(fOk);

	if (g_HostCtxt.DebugDataSpaces)
	{
		g_HostCtxt.DebugDataSpaces->Release();
		g_HostCtxt.DebugDataSpaces = nullptr;
	}

	if (g_HostCtxt.DebugAdvanced)
	{
		g_HostCtxt.DebugAdvanced->Release();
		g_HostCtxt.DebugAdvanced = nullptr;
	}

	if (g_HostCtxt.DebugSymbols)
	{
		g_HostCtxt.DebugSymbols->Release();
		g_HostCtxt.DebugSymbols = nullptr;
	}

	if (g_HostCtxt.DebugSysObj)
	{
		g_HostCtxt.DebugSysObj->Release();
		g_HostCtxt.DebugSysObj = nullptr;
	}

	if (g_HostCtxt.DebugControl)
	{
		g_HostCtxt.DebugControl->Release();
		g_HostCtxt.DebugControl = nullptr;
	}

	if (g_HostCtxt.DebugClient)
	{
		g_HostCtxt.DebugClient->Release();
		g_HostCtxt.DebugClient = nullptr;
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
		elem = g_HostCtxt.ScriptPath;
		while (elem)
		{
			ScriptPathElem* next = elem->Next;
			delete elem;
			elem = next;
		}

		elem = g_HostCtxt.ScriptPath = nullptr;

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

			if (!g_HostCtxt.ScriptPath)
			{
				// Save the head.
				//
				g_HostCtxt.ScriptPath = elem;
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

		if (!g_HostCtxt.ScriptPath)
		{
			// Save the head.
			//
			g_HostCtxt.ScriptPath = elem;
			assert(!prev);
		}
		else
		{
			prev->Next = elem;
		}
	}

	// Print current path.
	//
	elem = g_HostCtxt.ScriptPath;
	while (elem)
	{
		hr = GetHostContext()->DebugControl->Output(
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
	DWORD startTime = 0;
	DWORD endTime = 0;

	if (!args[0])
	{
		// If it's an empty string, fail now.
		//
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: !runscript requires at least one argument.\n");
		hr = E_INVALIDARG;
		goto exit;
	}

	wszArgs = UtilConvertAnsiToWide(args);
	if (!wszArgs)
	{
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to convert args to wide string.\n");
		hr = E_FAIL;
		goto exit;
	}

	argList = CommandLineToArgvW(wszArgs, &cArgs);
	if (!argList)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to parse arguments: 0x%08x\n", hr);
		goto exit;
	}

	bool timeRun = false;
	int i = 0;
	for (i = 0; i < cArgs; ++i)
	{
		if (!wcscmp(argList[i], L"--"))
		{
			// Swallow and break out.
			//
			++i;
			break;
		}

		// Keep switches to ourselves.
		//
		if (argList[i][0] == L'-')
		{
			if (!wcscmp(argList[i], L"-t"))
			{
				timeRun = true;
			}
		}
		else
		{
			break;
		}
	}

	// Rebase the arguments from the script to after what we consumed for the
	// host.
	//
	WCHAR** argsForScriptProv = &argList[i];
	int cArgsForScriptProv = cArgs - i;
	assert(cArgsForScriptProv >= 0);

	startTime = GetTickCount();

	// We should examine the extension of the script and walk the list
	// of registered providers to find the first one that claims the extension.
	// For now, we only have one provider.
	//
	hr = GetHostContext()->ScriptProvider->Run(cArgsForScriptProv, argsForScriptProv);
	if (FAILED(hr))
	{
		goto exit;
	}

	endTime = GetTickCount();

	if (timeRun)
	{
		DWORD elapsedMs = endTime - startTime;
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_NORMAL, "\nExecution time: %.2f s\n", elapsedMs / 1000.0);
	}

exit:

	// Reset buffering flag, in case script forgets (or has an exception.)
	//
	GetHostContext()->IsBuffering = 0;

	// Flush any remaining buffer (in case an exception was raised in the script,
	// or they failed to stop buffering.
	//
	UtilFlushMessageBuffer(GetHostContext());

	if (FAILED(hr))
	{
		GetHostContext()->DebugControl->Output(DEBUG_OUTPUT_ERROR, "Script failed: 0x%08x.\n", hr);
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
	hr = GetHostContext()->ScriptProvider->RunString(args);
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	if (FAILED(hr))
	{
		GetHostContext()->DebugControl->Output(DEBUG_OUTPUT_ERROR, "Script failed: 0x%08x.\n", hr);
	}
	return hr;
}



