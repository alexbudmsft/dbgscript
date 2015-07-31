#pragma once

#include "common.h"

_Check_return_ HRESULT
UtilReadPointer(
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal);

_Check_return_ bool
UtilCheckAbort();

_Check_return_ WCHAR*
UtilConvertAnsiToWide(
	_In_z_ const char* ansiStr);

_Check_return_ bool
UtilFileExists(
	_In_z_ const WCHAR* wszPath);