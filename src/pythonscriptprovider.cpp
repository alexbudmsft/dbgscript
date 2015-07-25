#include <python.h>
#include "../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
#include "common.h"

static const char* x_ModuleName = "dbgscript";

struct DbgScriptOut
{
	PyObject_HEAD
};

struct ThreadObj
{
	PyObject_HEAD
};

PyObject* g_DbgScriptOut;
PyObject* g_Thread;

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

static PyObject*
Thread_get_teb(
	_In_ PyObject* /* self */,
	_In_opt_ void* /* closure */)
{
	PyObject* ret = nullptr;

	// Get TEB from debug client.
	//
	UINT64 teb = 0;
	HRESULT hr = GetDllGlobals()->DebugSysObj->GetCurrentThreadTeb(&teb);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get TEB. Error 0x%08x.", hr);
		goto exit;
	}

	ret = PyLong_FromUnsignedLongLong(teb);

exit:
	return ret;
}

// Attribute is read-only.
//
static int
Thread_set_teb(PyObject* /* self */, PyObject* /* value */, void* /* closure */)
{
	PyErr_SetString(PyExc_AttributeError, "readonly attribute");
	return -1;
}

static PyGetSetDef Thread_GetSetDef[] = 
{
	{
		"teb",
		Thread_get_teb,
		Thread_set_teb,
		"first name",
		NULL
	},
	{ NULL }  /* Sentinel */
};

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

static PyTypeObject ThreadType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.ThreadType",     /* tp_name */
	sizeof(ThreadObj)       /* tp_basicsize */
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

static void
initThreadType()
{
	ThreadType.tp_flags = Py_TPFLAGS_DEFAULT;
	ThreadType.tp_doc = PyDoc_STR("dbgscript.Thread objects");
	ThreadType.tp_getset = Thread_GetSetDef;
	ThreadType.tp_new = PyType_GenericNew;
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

	initThreadType();

	// Finalize the type definition.
	//
	if (PyType_Ready(&ThreadType) < 0)
	{
		return nullptr;
	}

	// Alloc a single instance of the DbgScriptOutType object. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	g_Thread = ThreadType.tp_new(&ThreadType, nullptr, nullptr);
	if (!g_Thread)
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

	Py_INCREF(g_Thread);
	PyModule_AddObject(module, "Thread", g_Thread);

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


