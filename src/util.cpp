#include "util.h"

// Read a pointer from the target's memory.
//
_Check_return_ HRESULT
UtilReadPointer(
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal)
{
	return GetDllGlobals()->DebugDataSpaces->ReadPointersVirtual(1, addr, ptrVal);
}

