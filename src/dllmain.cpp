//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dllmain.cpp
// @Author: alexbud
//
// Purpose:
//
//  Main implementation of host DLL (dbgscript.dll)
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include <strsafe.h>
#include <assert.h>
#include <crtdbg.h>

#include "common.h"
#include "cmdline.h"
#include "support/util.h"

static DbgScriptHostContext g_HostCtxt;

_Check_return_ DbgScriptOutputCallbacks*
GetDbgScriptOutputCb();

//------------------------------------------------------------------------------
// Function: GetHostContext
//
// Description:
//
//  Get pointer to DbgScriptHostContext singleton.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ DbgScriptHostContext*
GetHostContext()
{
	return &g_HostCtxt;
}


// ScriptProviderInfo - Represents information about a Script Provider.
//
struct ScriptProviderInfo
{
	ScriptProviderInfo()
	{
		memset(this, 0, sizeof(*this));
	}
	
	// Next pointer in list.
	//
	ScriptProviderInfo* Next;

	// Path to DLL.
	//
	WCHAR DllFileName[MAX_PATH + 1];

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
	
	// Pointer to script provider instance (created via factory).
	//
	IScriptProvider* ScriptProvider;
	
	// Language ID this provider responds to (and thus claims).
	//
	WCHAR LangId[MAX_LANG_ID]; // -l <lang>

	//
	// CRT memory leak debugging information.
	//
	
	_CrtMemState MemStateBefore;

	_CrtMemState MemStateAfter;

	_CrtMemState MemStateDiff;
};

// ScriptProvCallbackBinding - structure to encapsulate a callback's export
// with a field to populate in ScriptProviderInfo.
//
struct ScriptProvCallbackBinding
{
	// ExportName - Name of routine to bind.
	//
	const char* ExportName;

	// Offset in ScriptProviderInfo of function pointer to which callback
	// is to be assigned.
	//
	int CallbackOffset;
};

// Functions to be bound with their offsets in the structure.
//
static const ScriptProvCallbackBinding x_CallbackBindings[] =
{
	{ SCRIPT_PROV_INIT, offsetof(ScriptProviderInfo, InitFunc) },
	{ SCRIPT_PROV_CLEANUP, offsetof(ScriptProviderInfo, CleanupFunc) },
	{ SCRIPT_PROV_CREATE, offsetof(ScriptProviderInfo, CreateFunc) },
};

//------------------------------------------------------------------------------
// Function: DllMain
//
// Description:
//
//  DLL Entry point for host layer (dbgscript.dll).
//
// Parameters:
//
// Returns:
//
// Notes:
//
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

//------------------------------------------------------------------------------
// Function: unloadScriptProvider
//
// Description:
//
//  Unload the given script provider.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static void
unloadScriptProvider(
	_Inout_ ScriptProviderInfo* info)
{
	if (info->ScriptProvider)
	{
		assert(info->Module);
		
		info->ScriptProvider->Cleanup();
		info->ScriptProvider = nullptr;
	
		// Call DLL cleanup routine.
		//
		info->CleanupFunc();
		
		// Snapshot the CRT memory state just before unloading the DLL so
		// we get a good stack.
		//
		_CrtMemCheckpoint(&info->MemStateAfter);

		_CrtMemDifference(
			&info->MemStateDiff,
			&info->MemStateBefore,
			&info->MemStateAfter);

		_CrtMemDumpAllObjectsSince(&info->MemStateBefore);
		
		// Unload the module.
		//
		BOOL fOk = FreeLibrary(info->Module);
		assert(fOk);
		fOk;  // reference in retail.

		// Again after freeing the library.
		//
		_CrtMemDumpAllObjectsSince(&info->MemStateBefore);

		info->Module = nullptr;
		
		info->CreateFunc = nullptr;
		info->CleanupFunc = nullptr;
		info->InitFunc = nullptr;

		// DllFileName is still valid.
		//
	}
}

//------------------------------------------------------------------------------
// Function: loadAndCreateScriptProvider
//
// Description:
//
//  Load the given script provider and create an instance of it via its factory.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static _Check_return_ HRESULT
loadAndCreateScriptProvider(
	_Inout_ ScriptProviderInfo* info)
{
	HRESULT hr = S_OK;

	WCHAR dllPath[MAX_PATH] = {};
	StringCchCopy(STRING_AND_CCH(dllPath), info->DllFileName);
	WCHAR* lastBackSlash = wcsrchr(dllPath, L'\\');
	if (!lastBackSlash)
	{
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Invalid DLL Path '%ls' for provider '%ls'\n",
			info->DllFileName,
			info->LangId);
		hr = E_INVALIDARG;
		goto exit;
	}

	// Null out the slash to get only the directory path of the DLL.
	//
	*lastBackSlash = 0;

	BOOL fOk = SetDllDirectory(dllPath);
	if (!fOk)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	// Snapshot the CRT memory state before loading the DLL.
	//
	_CrtMemCheckpoint(&info->MemStateBefore);
	
	info->Module = LoadLibrary(info->DllFileName);
	if (!info->Module)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
	}

	// Bind all the callbacks.
	//
	for (int i = 0; i < _countof(x_CallbackBindings); ++i)
	{
		DWORD_PTR* storeLoc = (DWORD_PTR*)((BYTE*)info + x_CallbackBindings[i].CallbackOffset);
		*storeLoc = (DWORD_PTR)GetProcAddress(info->Module, x_CallbackBindings[i].ExportName);

		if (!*storeLoc)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto exit;
		}
	}

	// Initialize the provider DLL.
	//
	info->InitFunc(&g_HostCtxt);

	// For now we only have one.
	//
	info->ScriptProvider = info->CreateFunc();
	if (!info->ScriptProvider)
	{
		// Handle error.
		//
		hr = E_FAIL;
		goto exit;
	}

	// Call provider instance's init routine.
	//
	hr = info->ScriptProvider->Init();
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: loadScriptProviderIfNeeded
//
// Description:
//
//  Load given script provider if not already loaded.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static _Check_return_ HRESULT
loadScriptProviderIfNeeded(
	_Inout_ ScriptProviderInfo* info)
{
	if (!info->ScriptProvider)
	{
		return loadAndCreateScriptProvider(info);
	}
	return S_OK;
}

//------------------------------------------------------------------------------
// Function: unloadAllScriptProviders
//
// Description:
//
//  Unload the script providers, but keep the list in tact.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static void
unloadAllScriptProviders()
{
	ScriptProviderInfo* cur = g_HostCtxt.ScriptProviders;
	while (cur)
	{
		unloadScriptProvider(cur);
		cur = cur->Next;
	}
}

//------------------------------------------------------------------------------
// Function: cleanupScriptProviders
//
// Description:
//
//  Unload and destroy all script providers. Used at DLL shutdown.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static void
cleanupScriptProviders()
{
	ScriptProviderInfo* cur = g_HostCtxt.ScriptProviders;
	while (cur)
	{
		// Capture next before we destroy it.
		//
		ScriptProviderInfo* next = cur->Next;

		unloadScriptProvider(cur);
		
		// Free the elem in the list.
		//
		delete cur;
		cur = next;
	}

	g_HostCtxt.ScriptProviders = nullptr;
}

//------------------------------------------------------------------------------
// Function: registerScriptProviders
//
// Description:
//
//  Builds list of script providers based on registry. Does not actually
//  load them.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static _Check_return_ HRESULT
registerScriptProviders()
{
	HRESULT hr = S_OK;
	static const WCHAR* x_ProviderKey = L"Software\\Microsoft\\DbgScript\\Providers";
	HKEY hKey = nullptr;
	LONG ret = RegOpenKey(HKEY_CURRENT_USER, x_ProviderKey, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		if (ret == ERROR_FILE_NOT_FOUND)
		{
			GetHostContext()->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: Registry key 'HKCU\\%ls' not found. Did you install the providers?\n", x_ProviderKey);
		}
		hr = HRESULT_FROM_WIN32(ret);
		goto exit;
	}
	DWORD valType = 0;
	WCHAR valName[64];
	WCHAR data[MAX_PATH];
	DWORD cbData = 0;
	DWORD cchValName = 0;
	DWORD idx = 0;
	do
	{
		cbData = sizeof(data);
		cchValName = _countof(valName);

		ret = RegEnumValue(hKey, idx++, valName, &cchValName, nullptr, &valType, (BYTE*)data, &cbData);
		if (ret == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (ret != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(ret);
			goto exit;
		}

		// Validate type.
		//
		if (valType != REG_SZ)
		{
			hr = E_INVALIDARG;
			GetHostContext()->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: Provider value '%ls' not REG_SZ.\n", valName);
			goto exit;
		}

		// data is the path to the DLL
		// valName is the language token for the -l <lang> switch.
		//
		ScriptProviderInfo* head = g_HostCtxt.ScriptProviders;
		ScriptProviderInfo* info = new ScriptProviderInfo;
		StringCchCopy(STRING_AND_CCH(info->LangId), valName);
		StringCchCopy(STRING_AND_CCH(info->DllFileName), data);
		info->Next = head;

		// Prepend to head.
		//
		g_HostCtxt.ScriptProviders = info;

	} while (ret != ERROR_NO_MORE_ITEMS);

	if (!g_HostCtxt.ScriptProviders)
	{
		// No providers found!
		//
		hr = E_INVALIDARG;
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: No script providers found!\n");
		goto exit;
	}
exit:
	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = nullptr;
	}
	return hr;
}

//------------------------------------------------------------------------------
// Function: DebugExtensionInitialize
//
// Description:
//
//  dbgeng callback called at DLL initialization time. Initializes dbgscript DLL.
//
// Parameters:
//
// Returns:
//
// Notes:
//
DLLEXPORT HRESULT 
DebugExtensionInitialize(
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

	// Initialize all registered script providers.
	//
	hr = registerScriptProviders();
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
	// Here all the registered engines should be initialized.
	//
	return hr;
}

//------------------------------------------------------------------------------
// Function: DebugExtensionUninitialize
//
// Description:
//
//  dbgeng callback called at DLL cleanup time. Uninitializes dbgscript DLL.
//
// Parameters:
//
// Returns:
//
// Notes:
//
DLLEXPORT void CALLBACK 
DebugExtensionUninitialize()
{
	cleanupScriptProviders();

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

//------------------------------------------------------------------------------
// Function: scriptpath
//
// Synopsis:
//
//  !scriptpath [path]
//
// Description:
//
//  Sets or displays the current script path. 'path' is a comma-separated list
//  of paths to search for scripts in. If run with no parameters, prints the
//  current path.
//
// Returns:
//
// Notes:
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

//------------------------------------------------------------------------------
// Function: findScriptProvider
//
// Description:
//
//  Look up a script provider by its language id in the list of registered
//  providers.
//
// Parameters:
//
// Returns:
//
// Notes:
//
static _Check_return_ HRESULT
findScriptProvider(
	_In_opt_z_ const WCHAR* langId,
	_Outptr_ ScriptProviderInfo** scriptProv)
{
	HRESULT hr = S_OK;

	// Validated during DLL initalization that we found at least one provider.
	//
	assert(GetHostContext()->ScriptProviders);

	// Default to the head of the list.
	//
	*scriptProv = GetHostContext()->ScriptProviders;
	assert(scriptProv);

	if (langId)
	{
		ScriptProviderInfo* cur = GetHostContext()->ScriptProviders;
		bool found = false;
		while (cur)
		{
			if (!wcscmp(cur->LangId, langId))
			{
				*scriptProv = cur;
				found = true;
				break;
			}

			cur = cur->Next;
		}

		if (!found)
		{
			hr = E_INVALIDARG;
			GetHostContext()->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: No script provider with lang ID '%ls' found.\n", langId);
			goto exit;
		}
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: runscript
//
// Synopsis:
//
//  !runscript [host args] [--] [provider args] <scriptfile> [script args]
//
// Description:
//
//  Run a script against a given script provider.
//
//  [host args] can be
//
//  -l <lang id>  - specifies the language id, and thus script provider to
//                  invoke.
//  -t            - display the execution time at the end of the run.
//
//  '--' can be used to terminate the parsing of arguments by the host layer.
//
//  [provider args] are arguments that are specific to each script provider.
//
//  <scriptfile>  - names the script file to run. This can be an absolute path
//                  or just a filename. In the latter case it will be searched
//                  for in the cwd and in the list of paths specified by
//                  !scriptpath. (This parameter is required.)
//
//  [script args] are any optional arguments to be passed to the script itself.
//  
// Returns:
//
// Notes:
//
DLLEXPORT HRESULT CALLBACK
runscript(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         args)
{
	HRESULT hr = S_OK;
	DWORD startTime = 0;
	DWORD endTime = 0;
	ParsedArgs parsedArgs = {};
	ScriptProviderInfo* scriptProv = nullptr;
	const bool startVMEnabled = GetHostContext()->StartVMEnabled;
	bool initializedProvider = false;
	char* argsMutable = nullptr;
	WCHAR* wszArgs = nullptr;
	int cArgs = 0;
	WCHAR** argList = nullptr;
	DbgScriptHostContext* hostCtxt = GetHostContext();
	if (!args[0])
	{
		// If it's an empty string, fail now.
		//
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: !runscript requires at least one argument.\n");
		hr = E_INVALIDARG;
		goto exit;
	}
	argsMutable = _strdup(args);
	hr = ParseArgs(argsMutable, &parsedArgs);
	if (FAILED(hr))
	{
		goto exit;
	}

	startTime = GetTickCount();

	hr = findScriptProvider(parsedArgs.LangId, &scriptProv);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = loadScriptProviderIfNeeded(scriptProv);
	if (FAILED(hr))
	{
		goto exit;
	}
	
	initializedProvider = true;

	wszArgs = UtilConvertAnsiToWide(parsedArgs.RemainingArgs);
	if (!wszArgs)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to convert args to wide string.\n");
		hr = E_OUTOFMEMORY;
		goto exit;
	}
	
	// Generate an argument vector for script execution.
	//
	argList = CommandLineToArgvW(wszArgs, &cArgs);
	if (!argList)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to parse arguments: 0x%08x\n", hr);
		goto exit;
	}

	// Execute the script.
	//
	hr = scriptProv->ScriptProvider->Run(cArgs, argList);
	if (FAILED(hr))
	{
		goto exit;
	}

	endTime = GetTickCount();

	// Report timing.
	//
	if (parsedArgs.TimeRun)
	{
		DWORD elapsedMs = endTime - startTime;
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_NORMAL, "\nExecution time: %.2f s\n", elapsedMs / 1000.0);
	}

exit:
	//
	// Cleanup.
	//
	
	if (!startVMEnabled && initializedProvider)
	{
		unloadScriptProvider(scriptProv);
	}

	// Reset buffering flag, in case script forgets (or has an exception.)
	//
	hostCtxt->IsBuffering = 0;

	// Flush any remaining buffer (in case an exception was raised in the script,
	// or they failed to stop buffering.
	//
	UtilFlushMessageBuffer(GetHostContext());

	// Print final failure message *after* we flush the buffer.
	//
	if (FAILED(hr))
	{
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR, "Script failed: 0x%08x.\n", hr);
	}

	// Free memory.
	// NULL is safe to use with all these functions.
	//
	delete[] wszArgs;
	LocalFree(argList);
	free(argsMutable);

	return hr;
}

//------------------------------------------------------------------------------
// Function: evalstring
//
// Synopsis:
//
//  !evalstring [host args] [--] <string-to-eval>
//
// Description:
//
//  Evaluates a string against a script provider.
//
//  [host args] are as in !runscript.
//
//  <string-to-eval> is the sting to evaluate. (This parameter is required.)
//
// Returns:
//
// Notes:
//
DLLEXPORT HRESULT CALLBACK
evalstring(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         args)
{
	HRESULT hr = S_OK;
	ParsedArgs parsedArgs = {};
	ScriptProviderInfo* scriptProv = nullptr;
	const bool startVMEnabled = GetHostContext()->StartVMEnabled;
	bool initializedProvider = false;
	char* argsMutable = nullptr;

	if (!args[0])
	{
		// If it's an empty string, fail now.
		//
		GetHostContext()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: !evalstring requires at least one argument.\n");
		hr = E_INVALIDARG;
		goto exit;
	}
	
	argsMutable = _strdup(args);
	hr = ParseArgs(argsMutable, &parsedArgs);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = findScriptProvider(parsedArgs.LangId, &scriptProv);
	if (FAILED(hr))
	{
		goto exit;
	}
	
	// Skip empty strings.
	//
	if (!*parsedArgs.RemainingArgs)
	{
		goto exit;
	}
	
	hr = loadScriptProviderIfNeeded(scriptProv);
	if (FAILED(hr))
	{
		goto exit;
	}
	
	initializedProvider = true;

	g_HostCtxt.DebugControl->Output(
		DEBUG_OUTPUT_VERBOSE,
		"Evaluating string '%s'.\n", parsedArgs.RemainingArgs);

	hr = scriptProv->ScriptProvider->RunString(parsedArgs.RemainingArgs);
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	if (!startVMEnabled && initializedProvider)
	{
		unloadScriptProvider(scriptProv);
	}
	
	if (FAILED(hr))
	{
		GetHostContext()->DebugControl->Output(DEBUG_OUTPUT_ERROR, "Script failed: 0x%08x.\n", hr);
	}
	
	free(argsMutable);
	return hr;
}

//------------------------------------------------------------------------------
// Function: startvm
//
// Synopsis:
//
//  !startvm
//
// Description:
//
//  Starts a persistent interpreter VM session. By default, after script
//  execution, the script provider is unloaded and thus all its state is cleared.
//
//  This command instructs dbgscript to retain the VM state after each execution,
//  until !stopvm is called.
//
// Returns:
//
// Notes:
//
DLLEXPORT HRESULT CALLBACK
startvm(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         /*args*/)
{
	HRESULT hr = S_OK;

	if (GetHostContext()->StartVMEnabled)
	{
		g_HostCtxt.DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: VM already started. Use !stopvm to end it.\n");
		hr = E_INVALIDARG;
		goto exit;
	}
	else
	{
		GetHostContext()->StartVMEnabled = true;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: stopvm
//
// Synopsis:
//
//  !stopvm
//
// Description:
//
//  Stops the VM session started by !startvm.
//  
// Returns:
//
// Notes:
//
DLLEXPORT HRESULT CALLBACK
stopvm(
	_In_     IDebugClient* /*client*/,
	_In_opt_ PCSTR         /*args*/)
{
	HRESULT hr = S_OK;
	
	if (!GetHostContext()->StartVMEnabled)
	{
		g_HostCtxt.DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: VM not started. Use !startvm to start it.\n");
		hr = E_INVALIDARG;
		goto exit;
	}
	else
	{
		// Call all provider instance cleanup routine.
		//
		unloadAllScriptProviders();
		GetHostContext()->StartVMEnabled = false;
	}
exit:
	return hr;
}

