#include "stackframe.h"
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
};

static PyMemberDef StackFrame_MemberDef[] =
{
	{ "frame_number", T_ULONG, offsetof(StackFrameObj, FrameNumber), READONLY },
	{ "instruction_offset", T_ULONGLONG, offsetof(StackFrameObj, InstructionOffset), READONLY },
	{ NULL }
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
	StackFrameType.tp_new = PyType_GenericNew;

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
	_In_ UINT64 instructionOffset)
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

	return obj;
}