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
	~CAutoSwitchStackFrame();
	
private:
	DbgScriptHostContext* m_HostCtxt;
	ULONG m_PrevIdx;
	bool m_DidSwitch;
};

class CAutoSetOutputCallback
{
public:
	CAutoSetOutputCallback(
		_In_ DbgScriptHostContext* hostCtxt,
		_In_ IDebugOutputCallbacks* cb);
		
	~CAutoSetOutputCallback();
	
 private:
	DbgScriptHostContext* m_HostCtxt;
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
	~CAutoSwitchThread();

private:
	DbgScriptHostContext* m_HostCtxt;
	ULONG m_PrevThreadId;
	bool m_DidSwitch;
};
