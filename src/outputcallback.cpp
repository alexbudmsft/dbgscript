#include "common.h"
#include <strsafe.h>

// Nothing private because this class is hidden in the .cpp file.
//
class DbgScriptOutputCallbacks : public IDebugOutputCallbacks
{
public:
	// IUnknown.
	//
	STDMETHOD(QueryInterface) (
		_In_ REFIID InterfaceId,
		_Out_ PVOID* Interface
		) override;
	STDMETHOD_(ULONG, AddRef) (
		) override;
	STDMETHOD_(ULONG, Release) (
		) override;

	// IDebugOutputCallbacks
	//
	STDMETHOD(Output) (
		_In_ ULONG mask,
		_In_z_ PCSTR text) override;

	char Buf[4096];
};

static DbgScriptOutputCallbacks s_DbgScriptOutputCb;

_Check_return_ DbgScriptOutputCallbacks*
GetDbgScriptOutputCb()
{
	return &s_DbgScriptOutputCb;
}

STDMETHODIMP
DbgScriptOutputCallbacks::Output(
	_In_ ULONG mask,
	_In_z_ PCSTR text)
{
	// Only care about normal output.
	//
	if (mask != DEBUG_OUTPUT_NORMAL)
	{
		return S_OK;
	}

	// Buffer the output.
	//
	StringCchCopyA(STRING_AND_CCH(Buf), text);

	return S_OK;
}

char*
DbgScriptOutCallbacksGetBuffer(
	_In_ DbgScriptOutputCallbacks* cb)
{
	return cb->Buf;
}

STDMETHODIMP
DbgScriptOutputCallbacks::QueryInterface(
	_In_ REFIID riid,
	_Out_ PVOID* ppv)
{
	if (riid == __uuidof(IDebugOutputCallbacks) || riid == IID_IUnknown)
	{
		// Cast not strictly necessary, but good habit in case we implement more
		// than one interface.
		//
		*ppv = static_cast<DbgScriptOutputCallbacks*>(this);
	}
	else
	{
		*ppv = nullptr;
	}

	if (*ppv)
	{
		// AddRef the interface we're about to give back.
		//
		reinterpret_cast<IUnknown*>(*ppv)->AddRef();

		return S_OK;
	}

	return E_NOINTERFACE;
}

// These are no-ops because we allocate a single static object.
//
STDMETHODIMP_(ULONG)
DbgScriptOutputCallbacks::AddRef()
{
	return 0;
}

STDMETHODIMP_(ULONG)
DbgScriptOutputCallbacks::Release()
{
	return 0;
}
