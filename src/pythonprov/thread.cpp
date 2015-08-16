#include "thread.h"
#include "stackframe.h"
#include <structmember.h>
#include "util.h"
#include "common.h"
#include <dsthread.h>

static PyMemberDef Thread_MemberDef [] = 
{
	{ "engine_id", T_ULONG, offsetof(ThreadObj, Thread.EngineId), READONLY },
	{ "thread_id", T_ULONG, offsetof(ThreadObj, Thread.ThreadId), READONLY },
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
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	PyObject* ret = nullptr;

	// Get TEB from debug client.
	//
	UINT64 teb = 0;
	HRESULT hr = hostCtxt->DebugSysObj->GetCurrentThreadTeb(&teb);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get TEB. Error 0x%08x.", hr);
		goto exit;
	}

	ret = PyLong_FromUnsignedLongLong(teb);

exit:
	return ret;
}

static PyObject*
Thread_get_current_frame(
	_In_ PyObject* self,
	_In_opt_ void* /* closure */)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	PyObject* ret = nullptr;
	DbgScriptStackFrame dsframe = {};

	// Call the support library to fill in the frame information.
	//
	HRESULT hr = DsGetCurrentStackFrame(hostCtxt, &dsframe);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsGetCurrentStackFrame failed. Error 0x%08x.", hr);
		goto exit;
	}
	
	ret = AllocStackFrameObj(&dsframe, reinterpret_cast<ThreadObj*>(self));

exit:
	return ret;
}

static PyObject*
Thread_get_stack(
	_In_ PyObject* self,
	_In_ PyObject* /* args */)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	ThreadObj* thd = (ThreadObj*)self;
	HRESULT hr = S_OK;

	PyObject* tuple = nullptr;

	DEBUG_STACK_FRAME frames[512];
	ULONG framesFilled = 0;

	hr = DsGetStackTrace(
		hostCtxt,
		&thd->Thread,
		frames,
		_countof(frames),
		&framesFilled);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsGetStackTrace failed. Error 0x%08x.", hr);
		goto exit;
	}

	tuple = PyTuple_New(framesFilled);

	// Build a tuple of StackFrame objects.
	//
	for (ULONG i = 0; i < framesFilled; ++i)
	{
		DbgScriptStackFrame frm;
		frm.FrameNumber = frames[i].FrameNumber;
		frm.InstructionOffset = frames[i].InstructionOffset;
		PyObject* frame = AllocStackFrameObj(&frm, thd);
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
		SetReadOnlyProperty,
		PyDoc_STR("Address of TEB"),
		NULL
	},
	{
		"current_frame",
		Thread_get_current_frame,
		SetReadOnlyProperty,
		PyDoc_STR("Current StackFrame object."),
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
	thd->Thread.EngineId = engineId;
	thd->Thread.ThreadId = threadId;

	return obj;
}
