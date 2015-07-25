#include <python.h>
#include "../../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
#include "process.h"
#include "thread.h"

static const char* x_ModuleName = "dbgscript";

struct DbgScriptOut
{
	PyObject_HEAD
};

PyObject* g_DbgScriptOut;

PyObject* 
DbgScriptOut_write(PyObject* /* self */, PyObject* args)
{
	const char* data = nullptr;
	if (!PyArg_ParseTuple(args, "s", &data))
	{
		return nullptr;
	}

	const size_t len = strlen(data);

	GetDllGlobals()->DebugControl->Output(DEBUG_OUTPUT_NORMAL, "%s", data);
	return PyLong_FromSize_t(len);
}

PyObject* DbgScriptOut_flush(PyObject* /*self*/, PyObject* /*args*/)
{
	// TODO: Is there a flush for IDebugControl?
	//
	Py_RETURN_NONE;
}

PyMethodDef g_DbgOutMethodsDef[] =
{
	{ "write", DbgScriptOut_write, METH_VARARGS, PyDoc_STR("write") },
	{ "flush", DbgScriptOut_flush, METH_VARARGS, PyDoc_STR("flush") },
	{ 0, 0, 0, 0 } // sentinel
};

static PyTypeObject DbgScriptOutType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.DbgScriptOutType",     /* tp_name */
	sizeof(DbgScriptOut)       /* tp_basicsize */
};

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

static void
initDbgScriptOutType()
{
	DbgScriptOutType.tp_flags = Py_TPFLAGS_DEFAULT;
	DbgScriptOutType.tp_doc = PyDoc_STR("dbgscript.DbgScriptOut objects");
	DbgScriptOutType.tp_methods = g_DbgOutMethodsDef;
	DbgScriptOutType.tp_new = PyType_GenericNew;
}


// Module initialization function for 'dbgscript'.
//
PyMODINIT_FUNC
PyInit_dbgscript()
{
	initDbgScriptOutType();

	// Finalize the type definition.
	//
	if (PyType_Ready(&DbgScriptOutType) < 0)
	{
		return nullptr;
	}

	// Alloc a single instance of the DbgScriptOutType object. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	g_DbgScriptOut = DbgScriptOutType.tp_new(&DbgScriptOutType, nullptr, nullptr);
	if (!g_DbgScriptOut)
	{
		return nullptr;
	}

	if (!InitThreadType())
	{
		return nullptr;
	}

	if (!InitProcessType())
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


