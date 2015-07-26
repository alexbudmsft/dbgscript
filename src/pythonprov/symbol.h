#pragma once

#include "../common.h"
#include <python.h>

_Check_return_ bool
InitSymbolType();

_Check_return_ PyObject*
AllocSymbolObj(
	_In_ ULONG size,
	_In_z_ const char* name,
	_In_z_ const char* type);