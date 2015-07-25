#pragma once

#include "../common.h"
#include <python.h>

_Check_return_ bool
InitStackFrameType();

_Check_return_ PyObject*
AllocStackFrameObj(
	_In_ ULONG frameNum);