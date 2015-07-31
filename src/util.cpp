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

// Checks debugger's abort bit.
//
_Check_return_ bool
UtilCheckAbort()
{
	HRESULT hr = GetDllGlobals()->DebugControl->GetInterrupt();

	// S_OK means user has issued an Ctrl-C or Ctrl-Break.
	//
	return hr == S_OK;
}

_Check_return_ WCHAR*
UtilConvertAnsiToWide(
	_In_z_ const char* ansiStr)
{
	const int cchBuf = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, nullptr, 0);
	WCHAR* wideBuf = new WCHAR[cchBuf];
	int ret = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideBuf, cchBuf);
	if (ret > 0)
	{
		return wideBuf;
	}
	else
	{
		return nullptr;
	}
}

_Check_return_ bool
UtilFileExists(
	_In_z_ const WCHAR* wszPath)
{
	const DWORD dwAttrib = GetFileAttributes(wszPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

