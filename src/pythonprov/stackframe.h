#pragma once

#include "../common.h"
#include <python.h>

struct ThreadObj;

_Check_return_ bool
InitStackFrameType();

_Check_return_ PyObject*
AllocStackFrameObj(
	_In_ DbgScriptStackFrame* frame,
	_In_ const ThreadObj* parentThread);
