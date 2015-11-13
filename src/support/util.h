//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.h
// @Author: alexbud
//
// Purpose:
//
//  Utility routines and classes for the DbgScript support library.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************
#pragma once

#include <assert.h>
#include <windows.h>
#include <dbgeng.h>
#include <hostcontext.h>
#include "../common.h"
#include <dsthread.h>
#include <dserrors.h>

// Max length of string we support in read{wide}string APIs in characters.
//
const int MAX_READ_STRING_LEN = 2048;

_Check_return_ HRESULT
UtilReadPointer(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal);

_Check_return_ HRESULT
UtilGetPeb(
	_In_ DbgScriptHostContext* hostCtxt,
	_Out_ UINT64* pebAddr);

_Check_return_ HRESULT
UtilGetFieldOffset(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* type,
	_In_z_ const char* field,
	_Out_ ULONG* offset);

_Check_return_ HRESULT
UtilGetNearestSymbol(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_(MAX_SYMBOL_NAME_LEN) char* buf);

_Check_return_ HRESULT
UtilReadWideString(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_to_(cchBuf, count) WCHAR* buf,
	_In_ ULONG cchBuf,
	_In_ int cchMaxToRead);

_Check_return_ HRESULT
UtilReadAnsiString(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_to_(cbBuf, cbCount) char* buf,
	_In_ ULONG cbBuf,
	_In_ int cbMaxToRead);

_Check_return_ HRESULT
UtilReadBytes(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_(cbCount) char* buf,
	_In_ ULONG cbCount,
	_Out_ ULONG* cbActualLen);

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

_Check_return_ HRESULT
UtilExecuteCommand(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* command);

_Check_return_ HRESULT
UtilFindScriptFile(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const WCHAR* scriptName,
	_Out_writes_(cchFullPath) WCHAR* fullPath,
	_In_ int cchFullPath);

class CAutoSwitchStackFrame
{
public:
	CAutoSwitchStackFrame(
		_In_ DbgScriptHostContext* hostCtxt,
		_In_ ULONG newIdx);
	
	_Check_return_ HRESULT
	Switch();
	
	~CAutoSwitchStackFrame();
	
private:
	DbgScriptHostContext* m_HostCtxt;
	ULONG m_TargetIdx;
	ULONG m_PrevIdx;
	bool m_DidSwitch;
};

class CAutoSetOutputCallback
{
public:
	CAutoSetOutputCallback(
		_In_ DbgScriptHostContext* hostCtxt,
		_In_ IDebugOutputCallbacks* cb);

	_Check_return_ HRESULT
	Install();
		
	~CAutoSetOutputCallback();
	
 private:
	DbgScriptHostContext* m_HostCtxt;
	IDebugOutputCallbacks* m_Target;
	IDebugOutputCallbacks* m_Prev;
};

// CAutoSwitchThread - Utility class to switch the debugger's thread context.
//
class CAutoSwitchThread
{
public:

	CAutoSwitchThread(
		_In_ DbgScriptHostContext* hostCtxt,
		_In_ const DbgScriptThread* thd);
	
	_Check_return_ HRESULT
	Switch();
	
	~CAutoSwitchThread();

private:
	DbgScriptHostContext* m_HostCtxt;
	ULONG m_TargetThreadId;
	ULONG m_PrevThreadId;
	bool m_DidSwitch;
};

typedef _Check_return_ HRESULT
(*EnumStackFrameVarsCb)(
	_In_ DEBUG_SYMBOL_ENTRY* entry,
	_In_z_ const char* symName,
	_In_ ULONG idx,
	_In_opt_ void* ctxt);

_Check_return_ HRESULT
UtilCountStackFrameVariables(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const DbgScriptThread* thd,
	_In_ const DbgScriptStackFrame* stackFrame,
	_In_ ULONG flags,
	_Out_ ULONG* numVars,
	_Out_ IDebugSymbolGroup2** symGrp);

_Check_return_ HRESULT
UtilEnumStackFrameVariables(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ IDebugSymbolGroup2* symGrp,
	_In_ ULONG numSym,
	_In_ EnumStackFrameVarsCb callback,
	_In_opt_ void* userctxt);

_Check_return_ HRESULT
UtilCountThreads(
	_In_ DbgScriptHostContext* hostCtxt,
	_Out_ ULONG* cThreads);

_Check_return_ HRESULT
UtilEnumThreads(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG cThreads,
	_Out_writes_(cThreads) ULONG* engineThreadIds,
	_Out_writes_(cThreads) ULONG* sysThreadIds);
