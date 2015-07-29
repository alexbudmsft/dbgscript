#include <python.h>
#include "../../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
#include "dbgscriptout.h"
#include "process.h"
#include "thread.h"
#include "stackframe.h"
#include "typedobject.h"

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
	if (!InitDbgScriptOutType())
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

static PyObject* g_DbgScriptOut;

// Module initialization function for 'dbgscript'.
//
PyMODINIT_FUNC
PyInit_dbgscript()
{
	if (!InitTypes())
	{
		return nullptr;
	}

	g_DbgScriptOut = AllocDbgScriptOutObj();
	if (!g_DbgScriptOut)
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

static void redirectStdOut()
{
	// Replace sys.stdout and sys.stderr with our new object.
	//
	PySys_SetObject("stdout", g_DbgScriptOut);
	PySys_SetObject("stderr", g_DbgScriptOut);
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
	redirectStdOut();
	return hr;
}

// Python is odd in that PySys_SetArgv takes a wide string, but PyRun_SimpleFile
// takes a narrow one.
//
_Check_return_ HRESULT
CPythonScriptProvider::Run(
	_In_z_ const char* szScriptName,
	_In_ int argc,
	_In_z_ WCHAR** argv)
{
	HRESULT hr = S_OK;
	FILE* fp = nullptr;
	fp = fopen(szScriptName, "r");
	if (!fp)
	{
		ULONG err = 0;
		_get_doserrno(&err);

		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Failed to open file '%s'. Error %d (%s).\n",
			szScriptName,
			err,
			strerror(errno));

		hr = HRESULT_FROM_WIN32(err);
		goto exit;
	}

	PySys_SetArgv(argc, argv);
	PyRun_SimpleFile(fp, szScriptName);

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


