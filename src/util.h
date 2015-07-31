#pragma once

#include "common.h"

_Check_return_ HRESULT
UtilReadPointer(
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal);

_Check_return_ bool
UtilCheckAbort();