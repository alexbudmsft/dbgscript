#pragma once

#include "../common.h"
#include <python.h>

struct ProcessObj;

_Check_return_ bool
InitTypedObjectType();

_Check_return_ PyObject*
AllocTypedObject(
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_z_ const char* type,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress);
