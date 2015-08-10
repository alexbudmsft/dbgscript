#include <python.h>

#include "pythonscriptprovider.h"
#include "util.h"
#include <strsafe.h>
#include "common.h"
#include "dbgscript.h"

CPythonScriptProvider::CPythonScriptProvider()
{}


_Check_return_ HRESULT
CPythonScriptProvider::StartVM()
{
	HRESULT hr = S_OK;
	PyImport_AppendInittab(x_DbgScriptModuleName, PyInit_dbgscript);
	WCHAR dllPath[MAX_PATH] = {};
	GetModuleFileName(GetPythonProvGlobals()->HModule, dllPath, _countof(dllPath));
	WCHAR* lastBackSlash = wcsrchr(dllPath, L'\\');
	assert(lastBackSlash);

	// Null out the slash to get only the directory path of the DLL.
	//
	*lastBackSlash = 0;

	// Set the PythonHome so that the standard libs are found beside the DLL.
	// (Lib\)
	//
	Py_SetPythonHome(dllPath);

	Py_Initialize();

	// Import 'dbgscript' module.
	//
	PyImport_ImportModule(x_DbgScriptModuleName);

	// Prevent sys.exit.
	//
	PySys_SetObject("exit", nullptr);

	return hr;
}

void
CPythonScriptProvider::StopVM()
{
	Py_Finalize();
}

_Check_return_ HRESULT
CPythonScriptProvider::Init()
{
	return StartVM();
}

static bool
runModule(
	_In_z_ const WCHAR* moduleName)
{
	bool ret = true;
	PyObject* runpy = nullptr;
	PyObject* runmodule = nullptr;
	PyObject* runargs = nullptr;
	PyObject* result = nullptr;
	
	runpy = PyImport_ImportModule("runpy");
	if (!runpy)
	{
		ret = false;
		goto exit;
	}

	runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
	if (!runmodule)
	{
		ret = false;
		goto exit;
	}

	runargs = Py_BuildValue("(u)", moduleName);
	if (!runargs)
	{
		ret = false;
		goto exit;
	}

	result = PyObject_Call(runmodule, runargs, nullptr);
	if (!result) 
	{
		ret = false;
		goto exit;
	}

exit:
	if (!ret)
	{
		PyErr_Print();
	}

	Py_XDECREF(runpy);
	Py_XDECREF(runmodule);
	Py_XDECREF(runargs);
	Py_XDECREF(result);
	return ret;
}

// Python is odd in that PySys_SetArgv takes a wide string, but PyRun_SimpleFile
// takes a narrow one.
//
_Check_return_ HRESULT
CPythonScriptProvider::Run(
	_In_ int argc,
	_In_ WCHAR** argv)
{
	char ansiScriptName[MAX_PATH] = {};
	WCHAR* moduleToRun = nullptr;
	HRESULT hr = S_OK;
	FILE* fp = nullptr;
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	int i = 0;

	// TODO: Generalize arg processing.
	//
	for (i = 0; i < argc; ++i)
	{
		// Terminate at first non-switch.
		//
		if (argv[i][0] != L'-')
		{
			break;
		}

		// We have a switch.
		//
		if (!wcscmp(argv[i], L"-m"))
		{
			if (i + 1 < argc)
			{
				// Advance to next token.
				//
				++i;

				moduleToRun = argv[i];
			}
			else
			{
				hr = E_INVALIDARG;
				hostCtxt->DebugControl->Output(
					DEBUG_OUTPUT_ERROR,
					"Error: -m expects a module name.\n");
				goto exit;
			}
		}
	}

	// The left over args go to the script itself.
	//
	WCHAR** argsForScript = &argv[i];
	int cArgsForScript = argc - i;
	assert(cArgsForScript >= 0);
	WCHAR fullScriptName[MAX_PATH];
	
	if (cArgsForScript)
	{
		hr = UtilFindScriptFile(
			hostCtxt,
			argsForScript[0],
			STRING_AND_CCH(fullScriptName));
		if (FAILED(hr))
		{
			goto exit;
		}
		
		// Replace first pointer with our final script name.
		//
		argsForScript[0] = fullScriptName;

		size_t cConverted = 0;

		// Convert to ANSI.
		//
		errno_t err = wcstombs_s(
			&cConverted, ansiScriptName, argsForScript[0], _countof(ansiScriptName));
		if (err)
		{
			GetPythonProvGlobals()->HostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: Failed to convert wide string to ANSI: %d.\n", err);
			hr = E_FAIL;
			goto exit;
		}

		if (moduleToRun)
		{
			// Module mode expects argv[0] to be the module name.
			//
			// Python makes a copy internally, so we can free this immediately.
			//
			const int cArgsForModuleMode = cArgsForScript + 1;
			WCHAR** argvForModuleMode = new WCHAR*[cArgsForModuleMode];
			argvForModuleMode[0] = moduleToRun;
			for (int j = 1; j < cArgsForModuleMode; ++j)
			{
				argvForModuleMode[j] = argsForScript[j - 1];
			}
			PySys_SetArgv(cArgsForModuleMode, argvForModuleMode);
			delete[] argvForModuleMode;
		}
		else
		{
			// Set up sys.argv.
			//
			PySys_SetArgv(cArgsForScript, argsForScript);
		}
	}

	if (moduleToRun)
	{
		if (!runModule(moduleToRun))
		{
			hr = E_FAIL;
			goto exit;
		}
	}
	else if (cArgsForScript)
	{
		fp = _wfopen(fullScriptName, L"r");
		if (!fp)
		{
			ULONG doserr = 0;
			_get_doserrno(&doserr);

			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Failed to open file '%ls'. Error %d (%s).\n",
				fullScriptName,
				doserr,
				strerror(errno));

			hr = HRESULT_FROM_WIN32(doserr);
			goto exit;
		}

		// Run the file.
		//
		PyRun_SimpleFile(fp, ansiScriptName);
	}

exit:
	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
	return hr;
}

_Check_return_ HRESULT
CPythonScriptProvider::RunString(
	_In_z_ const char* scriptString)
{
	HRESULT hr = S_OK;

	if (PyRun_SimpleString(scriptString) < 0)
	{
		hr = E_FAIL;
		goto exit;
	}

exit:
	return hr;
}

void
CPythonScriptProvider::Cleanup()
{
	StopVM();
	
	delete this;
}


_Check_return_ DLLEXPORT HRESULT
ScriptProviderInit(
	_In_ DbgScriptHostContext* hostCtxt)
{
	GetPythonProvGlobals()->HostCtxt = hostCtxt;
	return S_OK;
}

_Check_return_ DLLEXPORT void
ScriptProviderCleanup()
{
}

DLLEXPORT IScriptProvider*
ScriptProviderCreate()
{
	return new CPythonScriptProvider;
}
