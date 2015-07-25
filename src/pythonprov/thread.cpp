#include "thread.h"
#include <structmember.h>

struct ThreadObj
{
	PyObject_HEAD

	// Engine thread ID.
	//
	ULONG EngineId;

	// System Thread ID.
	//
	ULONG ThreadId;
};

static PyMemberDef Thread_MemberDef [] = 
{
	{ "engine_id", T_ULONG, offsetof(ThreadObj, EngineId), READONLY },
	{ "thread_id", T_ULONG, offsetof(ThreadObj, EngineId), READONLY },
	{ NULL }
};

static PyTypeObject ThreadType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.ThreadType",     /* tp_name */
	sizeof(ThreadObj)       /* tp_basicsize */
};

static PyObject* g_Thread;

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
		PyDoc_STR("Address of TEB"),
		NULL
	},
	{ NULL }  /* Sentinel */
};

_Check_return_ bool
InitThreadType()
{
	ThreadType.tp_flags = Py_TPFLAGS_DEFAULT;
	ThreadType.tp_doc = PyDoc_STR("dbgscript.Thread objects");
	ThreadType.tp_getset = Thread_GetSetDef;
	ThreadType.tp_members = Thread_MemberDef;
	ThreadType.tp_new = PyType_GenericNew;

	// Finalize the type definition.
	//
	if (PyType_Ready(&ThreadType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocThreadObj(
	_In_ ULONG engineId,
	_In_ ULONG threadId)
{
	PyObject* obj = nullptr;

	// Alloc a single instance of the DbgScriptOutType object. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = ThreadType.tp_new(&ThreadType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	// Set up fields.
	//
	ThreadObj* thd = (ThreadObj*)obj;
	thd->EngineId = engineId;
	thd->ThreadId = threadId;

	return obj;
}