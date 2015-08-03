#pragma once
#include <assert.h>
#include <windows.h>
#include <dbgeng.h>
#include <hostcontext.h>

_Check_return_ HRESULT
UtilReadPointer(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal);

_Check_return_ bool
UtilCheckAbort(
	_In_ DbgScriptHostContext* hostCtxt);

_Check_return_ WCHAR*
UtilConvertAnsiToWide(
	_In_z_ const char* ansiStr);

_Check_return_ bool
UtilFileExists(
	_In_z_ const WCHAR* wszPath);

void
UtilFlushMessageBuffer(
	_In_ DbgScriptHostContext* hostCtxt);

void
UtilBufferOutput(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* text,
	_In_ size_t len);

// TODO: Move impl. to .cpp and add error handling.
//
class CAutoSetOutputCallback
{
public:
	CAutoSetOutputCallback(
		_In_ DbgScriptHostContext* hostCtxt,
		_In_ IDebugOutputCallbacks* cb) :
		m_HostCtxt(hostCtxt)
	{
		IDebugClient* client = hostCtxt->DebugClient;
		HRESULT hr = client->GetOutputCallbacks(&m_Prev);
		assert(SUCCEEDED(hr));

		hr = client->SetOutputCallbacks(cb);
		assert(SUCCEEDED(hr));
	}
	~CAutoSetOutputCallback()
	{
		IDebugClient* client = m_HostCtxt->DebugClient;
		HRESULT hr = client->SetOutputCallbacks(m_Prev);
		assert(SUCCEEDED(hr));
	}
private:
	DbgScriptHostContext* m_HostCtxt;
	IDebugOutputCallbacks* m_Prev;
};