#include <python.h>
#include "../../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
#include "dbgscriptout.h"
#include "process.h"
#include "thread.h"
#include "stackframe.h"
#include "symbol.h"

static const char* x_ModuleName = "dbgscript";

// DbgScript Module Definition.
//
PyModuleDef g_ModuleDef =
{
	PyModuleDef_HEAD_INIT,
	x_ModuleName,  // Name
	PyDoc_STR("dbgscript Module"),       // Doc
	-1,  // size
	nullptr,  // Methods
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

	if (!InitSymbolType())
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
	PyObject* module = PyModule_Create(&g_ModuleDef);
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

_Check_return_ HRESULT
CPythonScriptProvider::Run(
	_In_z_ const char* scriptName)
{
	HRESULT hr = S_OK;
	FILE* fp = nullptr;
	fp = fopen(scriptName, "r");
	if (!fp)
	{
		ULONG err = 0;
		_get_doserrno(&err);

		GetDllGlobals()->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Failed to open file '%s'. Error %d (%s).\n",
			scriptName,
			err,
			strerror(errno));

		hr = HRESULT_FROM_WIN32(err);
		goto exit;
	}

	PyRun_SimpleFile(fp, scriptName);

exit:
	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
	return hr;
}

_Check_return_ void
CPythonScriptProvider::Cleanup()
{
	Py_Finalize();
}


