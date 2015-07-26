#include "thread.h"
#include "stackframe.h"
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
	"dbgscript.Thread",     /* tp_name */
	sizeof(ThreadObj)       /* tp_basicsize */
};

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

CAutoSwitchThread::CAutoSwitchThread(
	_In_ const ThreadObj* thd)
	:
	m_DidSwitch(false)
{
	// Get current thread id.
	//
	IDebugSystemObjects* sysObj = GetDllGlobals()->DebugSysObj;
	HRESULT hr = sysObj->GetCurrentThreadId(&m_PrevThreadId);
	assert(SUCCEEDED(hr));

	// Don't bother switching if we're already on the desired thread.
	//
	if (m_PrevThreadId != thd->EngineId)
	{
		hr = sysObj->SetCurrentThreadId(thd->EngineId);
		assert(SUCCEEDED(hr));
		m_DidSwitch = true;
	}
}

CAutoSwitchThread::~CAutoSwitchThread()
{
	// Revert to previous thread.
	//
	if (m_DidSwitch)
	{
		HRESULT hr = GetDllGlobals()->DebugSysObj->SetCurrentThreadId(m_PrevThreadId);
		assert(SUCCEEDED(hr));
	}
}

static PyObject*
Thread_get_stack(
	_In_ PyObject* self,
	_In_ PyObject* /* args */)
{
	// TODO: The bulk of this code is not Python-specific. Factor it out when
	// implementing Ruby provider.

	ThreadObj* thd = (ThreadObj*)self;
	HRESULT hr = S_OK;

	PyObject* tuple = nullptr;

	DEBUG_STACK_FRAME frames[512];
	ULONG framesFilled = 0;

	{
		// Need to switch thread context to 'this' thread, capture stack, then
		// switch back.
		//
		CAutoSwitchThread autoSwitchThd(thd);

		hr = GetDllGlobals()->DebugControl->GetStackTrace(
			0, 0, 0, frames, _countof(frames), &framesFilled);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to get stacktrace. Error 0x%08x.", hr);
			goto exit;
		}
	}

	assert(framesFilled > 0);

	tuple = PyTuple_New(framesFilled);

	// Build a tuple of Thread objects.
	//
	for (ULONG i = 0; i < framesFilled; ++i)
	{
		PyObject* frame = AllocStackFrameObj(i, frames[i].InstructionOffset, thd);
		if (!frame)
		{
			// Exception has already been setup by callee.
			//
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		if (PyTuple_SetItem(tuple, i, frame) != 0)
		{
			// Failed to set the item. Do not decref it. PyTuple_SetItem does
			// it internally.
			//
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}

exit:
	if (FAILED(hr))
	{
		// Release the tuple. This will release any held object inside the tuple.
		//
		Py_XDECREF(tuple);
		tuple = nullptr;
	}
	return tuple;
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

static PyMethodDef Thread_MethodDef[] =
{
	{
		"get_stack",
		Thread_get_stack,
		METH_NOARGS,
		PyDoc_STR("Return the callstack as a tuple of StackFrame objects.")
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
	ThreadType.tp_methods = Thread_MethodDef;
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