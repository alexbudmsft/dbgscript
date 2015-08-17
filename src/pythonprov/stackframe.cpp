#include "stackframe.h"
#include "thread.h"
#include "typedobject.h"
#include <structmember.h>
#include "util.h"
#include "common.h"

struct StackFrameObj
{
	PyObject_HEAD

	// Language-independent stack frame.
	//
	DbgScriptStackFrame Frame;
	
	// Backpointer to the owning thread. Opaque.
	//
	const ThreadObj* Thread;
};

static _Check_return_ HRESULT
buildTupleFromLocals(
	_In_ DEBUG_SYMBOL_ENTRY* entry,
	_In_z_ const char* symName,
	_In_ ULONG idx,
	_In_opt_ void* ctxt)
{
	HRESULT hr = S_OK;
	PyObject* tuple = (PyObject*)ctxt;

	PyObject* typedObj = AllocTypedObject(
		entry->Size,
		symName,
		entry->TypeId,
		entry->ModuleBase,
		entry->Offset);
	if (!typedObj)
	{
		// Exception has already been setup by callee.
		//
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	if (PyTuple_SetItem(tuple, idx, typedObj) != 0)
	{
		// Failed to set the item. Do not decref it. PyTuple_SetItem does
		// it internally.
		//
		hr = E_OUTOFMEMORY;
		goto exit;
	}
exit:
	return hr;
}

static PyObject*
getVariablesHelper(
	_In_ StackFrameObj* stackFrame,
	_In_ ULONG flags)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	PyObject* tuple = nullptr;
	HRESULT hr = S_OK;
	ULONG numSym = 0;
	IDebugSymbolGroup2* symGrp = nullptr;

	hr = UtilCountStackFrameVariables(
		hostCtxt,
		&stackFrame->Thread->Thread,
		&stackFrame->Frame,
		flags,
		&numSym,
		&symGrp);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "UtilCountStackFrameVariables failed. Error: 0x%08x", hr);
		goto exit;
	}
	
	tuple = PyTuple_New(numSym);
	
	hr = UtilEnumStackFrameVariables(
		hostCtxt,
		symGrp,
		numSym,
		buildTupleFromLocals,
		tuple);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "UtilEnumStackFrameVariables failed. Error: 0x%08x", hr);
		goto exit;
	}
	
exit:
	if (FAILED(hr))
	{
		// Release the tuple. This will release any held object inside the tuple.
		// Py_XDECREF checks for NULL.
		//
		Py_XDECREF(tuple);
		tuple = nullptr;
	}
	return tuple;
}

static PyObject*
StackFrame_get_locals(
	_In_ PyObject* self,
	_In_ PyObject* /* args */)
{
	StackFrameObj* stackFrame = (StackFrameObj*)self;
	return getVariablesHelper(stackFrame, DEBUG_SCOPE_GROUP_LOCALS);
}

static PyObject*
StackFrame_get_args(
	_In_ PyObject* self,
	_In_ PyObject* /* args */)
{
	StackFrameObj* stackFrame = (StackFrameObj*)self;
	return getVariablesHelper(stackFrame, DEBUG_SCOPE_GROUP_ARGUMENTS);
}

static void
StackFrame_dealloc(PyObject* self)
{
	StackFrameObj* stack = (StackFrameObj*)self;

	// Release the ref we took on the Thread Thread object.
	//
	Py_DECREF(stack->Thread);

	Py_TYPE(self)->tp_free(self);
}
static PyMemberDef StackFrame_MemberDef[] =
{
	{ "frame_number", T_ULONG, offsetof(StackFrameObj, Frame.FrameNumber), READONLY },
	{ "instruction_offset", T_ULONGLONG, offsetof(StackFrameObj, Frame.InstructionOffset), READONLY },
	{ NULL }
};

static PyMethodDef StackFrame_MethodDef[] =
{
	{
		"get_locals",
		StackFrame_get_locals,
		METH_NOARGS,
		PyDoc_STR("Return a tuple containing all the local variables and arguments in the stack frame.")
	},
	{
		"get_args",
		StackFrame_get_args,
		METH_NOARGS,
		PyDoc_STR("Return a tuple containing all the arguments (only) in the stack frame.")
	},
	{ NULL }  /* Sentinel */
};

static PyTypeObject StackFrameType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.StackFrame",     /* tp_name */
	sizeof(StackFrameObj)       /* tp_basicsize */
};

_Check_return_ bool
InitStackFrameType()
{
	StackFrameType.tp_flags = Py_TPFLAGS_DEFAULT;
	StackFrameType.tp_doc = PyDoc_STR("dbgscript.StackFrame objects");
	StackFrameType.tp_members = StackFrame_MemberDef;
	StackFrameType.tp_methods = StackFrame_MethodDef;
	StackFrameType.tp_new = PyType_GenericNew;
	StackFrameType.tp_dealloc = StackFrame_dealloc;

	// Finalize the type definition.
	//
	if (PyType_Ready(&StackFrameType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocStackFrameObj(
	_In_ DbgScriptStackFrame* frame,
	_In_ const ThreadObj* ThreadThread)
{
	PyObject* obj = nullptr;

	// Alloc a single instance of the StackFrameType class. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = StackFrameType.tp_new(&StackFrameType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	// Set up fields.
	//
	StackFrameObj* stackFrame = (StackFrameObj*)obj;
	stackFrame->Frame = *frame;

	// Take a ref on the Thread object we're going to store.
	//
	Py_INCREF(ThreadThread);
	stackFrame->Thread = ThreadThread;

	return obj;
}
