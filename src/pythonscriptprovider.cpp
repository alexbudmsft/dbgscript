#include <python.h>
#include "../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
#include "common.h"

static const char* x_ModuleName = "dbgscript";

struct DbgScriptOut
{
	PyObject_HEAD
	IDebugControl* DbgCtrl;
};

PyObject* g_dbgScriptOut;

PyObject* DbgScriptOut_write(PyObject* self, PyObject* args)
{
	DbgScriptOut* selfimpl = reinterpret_cast<DbgScriptOut*>(self);
	const char* data = nullptr;
	if (!PyArg_ParseTuple(args, "s", &data))
	{
		return nullptr;
	}

	const size_t len = strlen(data);

	// TODO: Write to debugger stream.
	//
	selfimpl->DbgCtrl->Output(DEBUG_OUTPUT_NORMAL, "%s", data);
	return PyLong_FromSize_t(len);
}

PyObject* DbgScriptOut_flush(PyObject* /*self*/, PyObject* /*args*/)
{
	// TODO: Is there a flush for IDebugControl?
	//
	Py_RETURN_NONE;
}

PyMethodDef g_MethodsDef[] =
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
	DbgScriptOutType.tp_methods = g_MethodsDef;
	DbgScriptOutType.tp_new = PyType_GenericNew;
}

static void
initDbgScriptOut(
	_Out_ DbgScriptOut* out)
{
	out->DbgCtrl = GetDllGlobals()->DebugControl;
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
	//
	g_dbgScriptOut = DbgScriptOutType.tp_new(&DbgScriptOutType, nullptr, nullptr);
	if (!g_dbgScriptOut)
	{
		return nullptr;
	}

	// Initialize g_dbgScriptOut.
	//
	initDbgScriptOut(reinterpret_cast<DbgScriptOut*>(g_dbgScriptOut));

	// Create a module object.
	//
	return PyModule_Create(&g_ModuleDef);
}

static void redirectStdOut()
{
	// Replace sys.stdout and sys.stderr with our new object.
	//
	PySys_SetObject("stdout", g_dbgScriptOut);
	PySys_SetObject("stderr", g_dbgScriptOut);
}

CPythonScriptProvider::CPythonScriptProvider(
	_In_z_ const WCHAR* scriptName) :
	m_ScriptName(scriptName)
{}

_Check_return_ HRESULT
CPythonScriptProvider::Init()
{
	HRESULT hr = S_OK;
	PyImport_AppendInittab(x_ModuleName, PyInit_dbgscript);

	Py_SetProgramName(const_cast<WCHAR*>(m_ScriptName));  /* optional but recommended */

	// Can optionally use Py_SetPythonHome to point to a place where the python
	// standard lib is deployed/installed. Use GetModuleFileName of the DLL
	// as the path so that we can deploy the Lib\ beside our DLL, not windbg.exe.
	//
	Py_Initialize();

	// Import 'dbgscript' module.
	//
	PyImport_ImportModule(x_ModuleName);
	redirectStdOut();
	// TEMP!
	//
	PyRun_SimpleString("from time import time,ctime\n"
		"print('Today is', ctime(time()))\n");

	return hr;
}

_Check_return_ void
CPythonScriptProvider::Cleanup()
{
	Py_Finalize();
}


