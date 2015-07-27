#pragma once
#include "../common.h"
#include <python.h>

struct ProcessObj;

_Check_return_ bool
InitProcessType();

_Check_return_ PyObject*
AllocProcessObj();

_Check_return_ const char*
ProcessObjGetModuleName(
	_In_ ProcessObj* proc,
	_In_ UINT64 modBase);