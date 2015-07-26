#include "stackframe.h"
#include "thread.h"
#include "symbol.h"
#include <structmember.h>

struct StackFrameObj
{
	PyObject_HEAD

	// Frame Number.
	//
	ULONG FrameNumber;

	// See DEBUG_STACK_FRAME::InstructionOffset.
	//
	UINT64 InstructionOffset;

	// Backpointer to the owning thread. Opaque.
	//
	const ThreadObj* Parent;
};

class CAutoSwitchStackFrame
{
public:
	CAutoSwitchStackFrame(
		_In_ ULONG newIdx);
	~CAutoSwitchStackFrame();
private:
	ULONG m_PrevIdx;
	bool m_DidSwitch;
};

CAutoSwitchStackFrame::CAutoSwitchStackFrame(
	_In_ ULONG newIdx) :
	m_PrevIdx((ULONG)-1),
	m_DidSwitch(false)
{
	IDebugSymbols3* dbgSymbols = GetDllGlobals()->DebugSymbols;
	
	HRESULT hr = dbgSymbols->GetCurrentScopeFrameIndex(&m_PrevIdx);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get symbol scope. Error 0x%08x.", hr);
		goto exit;
	}

	if (m_PrevIdx != newIdx)
	{
		hr = dbgSymbols->SetScopeFrameByIndex(newIdx);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to set symbol scope. Error 0x%08x.", hr);
			goto exit;
		}
		m_DidSwitch = true;
	}
exit:
	;
}

CAutoSwitchStackFrame::~CAutoSwitchStackFrame()
{
	if (m_DidSwitch)
	{
		IDebugSymbols3* dbgSymbols = GetDllGlobals()->DebugSymbols;
		if (m_PrevIdx != (ULONG)-1)
		{
			HRESULT hr = dbgSymbols->SetScopeFrameByIndex(m_PrevIdx);
			if (FAILED(hr))
			{
				PyErr_Format(PyExc_OSError, "Failed to set symbol scope. Error 0x%08x.", hr);
			}
		}
	}
}

static PyObject*
getVariablesHelper(
	_In_ StackFrameObj* stackFrame,
	_In_ ULONG flags)
{
	// TODO: The bulk of this code is not Python-specific. Factor it out when
	// implementing Ruby provider.
	//
	PyObject* tuple = nullptr;
	IDebugSymbols3* dbgSymbols = GetDllGlobals()->DebugSymbols;
	IDebugSymbolGroup2* symGrp = nullptr;
	HRESULT hr = S_OK;
	ULONG numSym = 0;

	{
		CAutoSwitchThread autoSwitchThd(stackFrame->Parent);
		CAutoSwitchStackFrame autoSwitchFrame(stackFrame->FrameNumber);
		if (PyErr_Occurred())
		{
			goto exit;
		}

		// Take a snapshot of the current symbols in this frame.
		//
		hr = dbgSymbols->GetScopeSymbolGroup2(flags, nullptr, &symGrp);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to create symbol group. Error 0x%08x.", hr);
			goto exit;
		}
	}

	hr = symGrp->GetNumberSymbols(&numSym);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get number of symbols. Error 0x%08x.", hr);
		goto exit;
	}

	tuple = PyTuple_New(numSym);

	// Build a tuple of Symbol objects.
	//
	for (ULONG i = 0; i < numSym; ++i)
	{
		ULONG symSize = 0;

		hr = symGrp->GetSymbolSize(i, &symSize);
		if (hr == E_NOINTERFACE)
		{
			// Sometimes variables are optimized away, which can cause this error.
			// Just leave the size at 0.
			//
		}
		else if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to get symbol size. Error 0x%08x.", hr);
			goto exit;
		}

		char symName[256];
		hr = symGrp->GetSymbolName(i, symName, _countof(symName), nullptr);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to get symbol name. Error 0x%08x.", hr);
			goto exit;
		}

		PyObject* symbol = AllocSymbolObj(symSize, symName);
		if (!symbol)
		{
			// Exception has already been setup by callee.
			//
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		if (PyTuple_SetItem(tuple, i, symbol) != 0)
		{
			// Failed to set the item. Do not decref it. PyTuple_SetItem does
			// it internally.
			//
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}


exit:
	if (symGrp)
	{
		symGrp->Release();
		symGrp = nullptr;
	}

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

	// Release the ref we took on the parent Thread object.
	//
	Py_DECREF(stack->Parent);

	Py_TYPE(self)->tp_free(self);
}
static PyMemberDef StackFrame_MemberDef[] =
{
	{ "frame_number", T_ULONG, offsetof(StackFrameObj, FrameNumber), READONLY },
	{ "instruction_offset", T_ULONGLONG, offsetof(StackFrameObj, InstructionOffset), READONLY },
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
	_In_ ULONG frameNum,
	_In_ UINT64 instructionOffset,
	_In_ const ThreadObj* parentThread)
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
	stackFrame->FrameNumber = frameNum;
	stackFrame->InstructionOffset = instructionOffset;

	// Take a ref on the Thread object we're going to store.
	//
	Py_INCREF(parentThread);
	stackFrame->Parent = parentThread;

	return obj;
}