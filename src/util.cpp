#include "util.h"
#include <assert.h>

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

// Flush any content in the message buffer.
//
void
UtilFlushMessageBuffer()
{
	DllGlobals* globals = GetDllGlobals();
	if (globals->BufPosition > 0)
	{
		// Because dbgeng expects null-terminated strings, terminate the buffer
		// at our current position marker.
		//
		// Last character is always reserved for the NULL terminator.
		// Position must be before that.
		//
		assert(globals->BufPosition <= _countof(globals->MessageBuf) - 1);
		globals->MessageBuf[globals->BufPosition] = 0;

		globals->DebugControl->Output(
			DEBUG_OUTPUT_NORMAL,
			"%s",
			globals->MessageBuf);

		// Reset position marker.
		//
		globals->BufPosition = 0;
	}
}

// Buffer the output if it fits, else flush the cache.
//
void
UtilBufferOutput(
	_In_z_ const char* text,
	_In_ size_t len)
{
	DllGlobals* globals = GetDllGlobals();
	assert(globals->IsBuffering > 0);

	// Last byte is reserved for null because windbg only accepts null-terminated
	// strings.
	//
	// If the write would end up trampling our NULL byte, we can't have that.
	// => Flush the current buffer and start anew.
	//
	if (globals->BufPosition + len > _countof(globals->MessageBuf) - 1)
	{
		UtilFlushMessageBuffer();

		assert(globals->BufPosition == 0);

		// If the string would never fit into the buffer even if it's empty,
		// truncate it.
		//
		// Assert in debug builds.
		//
		assert(len <= _countof(globals->MessageBuf) - 1);

		if (len > _countof(globals->MessageBuf) - 1)
		{
			len = _countof(globals->MessageBuf) - 1;
		}
	}

	// Append the string to our buffer.
	//
	memcpy(globals->MessageBuf + globals->BufPosition, text, len);
	globals->BufPosition += len;
	assert(globals->BufPosition <= _countof(globals->MessageBuf) - 1);
}