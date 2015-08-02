#pragma once
#include <assert.h>
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

void
UtilFlushMessageBuffer();

void
UtilBufferOutput(
	_In_z_ const char* text,
	_In_ size_t len);

// TODO: Move impl. to .cpp and add error handling.
//
class CAutoSetOutputCallback
{
public:
	CAutoSetOutputCallback(_In_ IDebugOutputCallbacks* cb)
	{
		IDebugClient* client = GetDllGlobals()->DebugClient;
		HRESULT hr = client->GetOutputCallbacks(&m_Prev);
		assert(SUCCEEDED(hr));

		hr = client->SetOutputCallbacks(cb);
		assert(SUCCEEDED(hr));
	}
	~CAutoSetOutputCallback()
	{
		IDebugClient* client = GetDllGlobals()->DebugClient;
		HRESULT hr = client->SetOutputCallbacks(m_Prev);
		assert(SUCCEEDED(hr));
	}
private:
	IDebugOutputCallbacks* m_Prev;
};