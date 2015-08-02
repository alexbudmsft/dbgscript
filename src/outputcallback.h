#pragma once
#include "common.h"

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

	const char*
	GetBuffer() const;

private:

	char Buf[4096];
};

_Check_return_ DbgScriptOutputCallbacks*
GetDbgScriptOutputCb();