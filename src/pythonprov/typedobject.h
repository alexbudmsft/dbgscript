#pragma once

#include "../common.h"
#include <python.h>

_Check_return_ bool
InitTypedObjectType();

_Check_return_ PyObject*
AllocTypedObject(
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_In_ bool wantPointer);
