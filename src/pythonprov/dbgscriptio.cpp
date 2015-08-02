#include "DbgScriptIO.h"
#include "util.h"
#include "common.h"

struct DbgScriptIOObj
{
	PyObject_HEAD
};

static PyObject*
DbgScriptIO_write(PyObject* /* self */, PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	const char* data = nullptr;
	if (!PyArg_ParseTuple(args, "s", &data))
	{
		return nullptr;
	}
	const size_t len = strlen(data);
	if (hostCtxt->IsBuffering > 0)
	{
		UtilBufferOutput(hostCtxt, data, len);
	}
	else
	{
		// Unbuffered case.
		//
		hostCtxt->DebugControl->Output(DEBUG_OUTPUT_NORMAL, "%s", data);
	}
	return PyLong_FromSize_t(len);
}

static PyObject*
DbgScriptIO_readline(PyObject* /* self */, PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	char buf[4096] = { 0 };
	ULONG actualLen = 0;

	// Set a default. Callers will typically call us with no parameters, expecting
	// a "typical" line to be read.
	//
	ULONG maxToRead = _countof(buf);

	if (!PyArg_ParseTuple(args, "|k:readline", &maxToRead))
	{
		return nullptr;
	}

	hostCtxt->DebugControl->Input(
		buf,
		min(maxToRead, _countof(buf)),
		&actualLen);

	return PyUnicode_FromString(buf);
}

static PyObject* 
DbgScriptIO_flush(PyObject* /*self*/, PyObject* /*args*/)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	hostCtxt->DebugClient->FlushCallbacks();

	Py_RETURN_NONE;
}

static PyMethodDef StackFrame_MethodDef[] =
{
	{ "write", DbgScriptIO_write, METH_VARARGS, PyDoc_STR("write") },
	{ "flush", DbgScriptIO_flush, METH_VARARGS, PyDoc_STR("flush") },
	{ "readline", DbgScriptIO_readline, METH_VARARGS, PyDoc_STR("readline") },
	{ 0, 0, 0, 0 } // sentinel
};

static PyTypeObject DbgScriptIOType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.DbgScriptIO",     /* tp_name */
	sizeof(DbgScriptIOObj)			/* tp_basicsize */
};

_Check_return_ bool
InitDbgScriptIOType()
{
	DbgScriptIOType.tp_flags = Py_TPFLAGS_DEFAULT;
	DbgScriptIOType.tp_doc = PyDoc_STR("dbgscript.DbgScriptIO objects");
	DbgScriptIOType.tp_methods = StackFrame_MethodDef;
	DbgScriptIOType.tp_new = PyType_GenericNew;

	// Finalize the type definition.
	//
	if (PyType_Ready(&DbgScriptIOType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocDbgScriptIOObj()
{
	// Alloc a single instance of the DbgScriptIOType class. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	return DbgScriptIOType.tp_new(&DbgScriptIOType, nullptr, nullptr);
}