#include <python.h>
#include "../../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
#include "dbgscriptio.h"
#include "process.h"
#include "thread.h"
#include "stackframe.h"
#include "typedobject.h"
#include "util.h"
#include <strsafe.h>

static const char* x_ModuleName = "dbgscript";

// Global function in the dbgscript module.
//
static PyObject*
DbgScript_execute_command(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	const char* command = nullptr;
	if (!PyArg_ParseTuple(args, "s:execute_command", &command))
	{
		return nullptr;
	}

	// CONSIDER: adding an option letting user control whether we echo the
	// command or not.
	//
	HRESULT hr = GetDllGlobals()->DebugControl->Execute(
		DEBUG_OUTCTL_ALL_CLIENTS,
		command,
		DEBUG_EXECUTE_ECHO | DEBUG_EXECUTE_NO_REPEAT);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to execute command '%s'. Error 0x%08x.", command, hr);
		goto exit;
	}

exit:
	Py_RETURN_NONE;
}

static PyMethodDef dbgscript_MethodsDef[] = 
{
	{
		"execute_command",
		DbgScript_execute_command,
		METH_VARARGS,
		PyDoc_STR("Execute a debugger command.")
	},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

// DbgScript Module Definition.
//
PyModuleDef dbgscript_ModuleDef =
{
	PyModuleDef_HEAD_INIT,
	x_ModuleName,  // Name
	PyDoc_STR("dbgscript Module"),       // Doc
	-1,  // size
	dbgscript_MethodsDef,  // Methods
};

_Check_return_ bool
InitTypes()
{
	// TODO: Make this a table of callbacks.
	//
	if (!InitDbgScriptIOType())
	{
		return false;
	}

	if (!InitThreadType())
	{
		return false;
	}

	if (!InitProcessType())
	{
		return false;
	}

	if (!InitStackFrameType())
	{
		return false;
	}

	if (!InitTypedObjectType())
	{
		return false;
	}
	return true;
}

static PyObject* g_DbgScriptIO;

// Module initialization function for 'dbgscript'.
//
PyMODINIT_FUNC
PyInit_dbgscript()
{
	if (!InitTypes())
	{
		return nullptr;
	}

	g_DbgScriptIO = AllocDbgScriptIOObj();
	if (!g_DbgScriptIO)
	{
		return nullptr;
	}

	// Create a module object.
	//
	PyObject* module = PyModule_Create(&dbgscript_ModuleDef);
	if (!module)
	{
		return nullptr;
	}

	static PyObject* s_ProcessObj = AllocProcessObj();
	if (!s_ProcessObj)
	{
		return nullptr;
	}

	Py_INCREF(s_ProcessObj);
	PyModule_AddObject(module, "Process", s_ProcessObj);
	
	return module;
}

static void 
redirectStreams()
{
	// Replace sys.stdout and sys.stderr with our new object.
	//
	PySys_SetObject("stdout", g_DbgScriptIO);
	PySys_SetObject("stderr", g_DbgScriptIO);
	PySys_SetObject("stdin", g_DbgScriptIO);
}

CPythonScriptProvider::CPythonScriptProvider()
{}

_Check_return_ HRESULT
CPythonScriptProvider::Init()
{
	HRESULT hr = S_OK;
	PyImport_AppendInittab(x_ModuleName, PyInit_dbgscript);
	WCHAR dllPath[MAX_PATH] = {};
	GetModuleFileName(GetDllGlobals()->HModule, dllPath, _countof(dllPath));
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
	PyImport_ImportModule(x_ModuleName);
	redirectStreams();
	return hr;
}

// Python is odd in that PySys_SetArgv takes a wide string, but PyRun_SimpleFile
// takes a narrow one.
//
_Check_return_ HRESULT
CPythonScriptProvider::Run(
	_In_ int argc,
	_In_z_ WCHAR** argv)
{
	char ansiScriptName[MAX_PATH];
	const WCHAR* moduleToRun = nullptr;
	HRESULT hr = S_OK;
	FILE* fp = nullptr;

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
				GetDllGlobals()->DebugControl->Output(
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
	WCHAR* scriptName = nullptr;

	size_t cConverted = 0;

	// Convert to ANSI.
	//
	errno_t err = wcstombs_s(
		&cConverted, ansiScriptName, argsForScript[0], _countof(ansiScriptName));
	if (err)
	{
		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to convert wide string to ANSI: %d.\n", err);
		hr = E_FAIL;
		goto exit;
	}

	// Try to find the script in the search locations provided by the extension.
	//
	WCHAR fullScriptName[MAX_PATH];
	scriptName = argsForScript[0];
	if (!UtilFileExists(scriptName))
	{
		// Try to search for the file in the script path list.
		//
		ScriptPathElem* elem = GetDllGlobals()->ScriptPath;
		while (elem)
		{
			StringCchPrintf(STRING_AND_CCH(fullScriptName), L"%ls\\%ls",
				elem->Path, scriptName);
			if (UtilFileExists(fullScriptName))
			{
				scriptName = fullScriptName;
				break;
			}
			elem = elem->Next;
		}
	}

	if (!UtilFileExists(scriptName))
	{
		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Script file not found in any of the search paths.\n");
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		goto exit;
	}

	// Replace first pointer with our final script name.
	//
	argsForScript[0] = scriptName;

	fp = _wfopen(scriptName, L"r");
	if (!fp)
	{
		ULONG doserr = 0;
		_get_doserrno(&doserr);

		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Failed to open file '%ls'. Error %d (%s).\n",
			scriptName,
			doserr,
			strerror(errno));

		hr = HRESULT_FROM_WIN32(err);
		goto exit;
	}

	// Set up sys.argv.
	//
	PySys_SetArgv(cArgsForScript, argsForScript);

	// Run the file.
	//
	PyRun_SimpleFile(fp, ansiScriptName);

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

_Check_return_ void
CPythonScriptProvider::Cleanup()
{
	Py_Finalize();
}


