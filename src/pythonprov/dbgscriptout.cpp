#include "dbgscriptout.h"

struct DbgScriptOutObj
{
	PyObject_HEAD
};

static PyObject*
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
DbgScriptOut_flush(PyObject* /*self*/, PyObject* /*args*/)
{
	// TODO: Is there a flush for IDebugControl?
	//
	Py_RETURN_NONE;
}

static PyMethodDef StackFrame_MethodDef[] =
{
	{ "write", DbgScriptOut_write, METH_VARARGS, PyDoc_STR("write") },
	{ "flush", DbgScriptOut_flush, METH_VARARGS, PyDoc_STR("flush") },
	{ 0, 0, 0, 0 } // sentinel
};

static PyTypeObject DbgScriptOutType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.DbgScriptOutType",     /* tp_name */
	sizeof(DbgScriptOutObj)			/* tp_basicsize */
};

_Check_return_ bool
InitDbgScriptOutType()
{
	DbgScriptOutType.tp_flags = Py_TPFLAGS_DEFAULT;
	DbgScriptOutType.tp_doc = PyDoc_STR("dbgscript.DbgScriptOut objects");
	DbgScriptOutType.tp_methods = StackFrame_MethodDef;
	DbgScriptOutType.tp_new = PyType_GenericNew;

	// Finalize the type definition.
	//
	if (PyType_Ready(&DbgScriptOutType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocDbgScriptOutObj()
{
	// Alloc a single instance of the DbgScriptOutType class. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	return DbgScriptOutType.tp_new(&DbgScriptOutType, nullptr, nullptr);
}