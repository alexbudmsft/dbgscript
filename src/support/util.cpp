//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.cpp
// @Author: alexbud
//
// Purpose:
//
//  Utilities provided by the support library.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "util.h"
#include <assert.h>
#include <strsafe.h>
#include "symcache.h"

//------------------------------------------------------------------------------
// Function: UtilReadPointer
//
// Description:
//
//  Read a pointer from the target's memory.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilReadPointer(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal)
{
	return hostCtxt->DebugDataSpaces->ReadPointersVirtual(1, addr, ptrVal);
}

//------------------------------------------------------------------------------
// Function: UtilGetFieldOffset
//
// Description:
//
//  Get field offset given a type and field. Mimics offsetof macro in C.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilGetFieldOffset(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* type,
	_In_z_ const char* field,
	_Out_ ULONG* offset)
{
	HRESULT hr = S_OK;
	
	// Lookup typeid/moduleBase from type name.
	//
	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, type);
	if (!typeInfo)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_TYPE_ID,
			type,
			hr);
		goto exit;
	}
	
	hr = hostCtxt->DebugSymbols->GetFieldOffset(
		typeInfo->ModuleBase,
		typeInfo->TypeId,
		field,
		offset);
	
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilGetNearestSymbol
//
// Description:
//
//  Lookup the nearest symbol given an address.
//
// Parameters:
//
//  addr - Virtual address to probe.
//  buf - On successful return, name of nearest symbol. Expected to be
//   a buffer of MAX_SYMBOL_NAME_LEN characters longs.
//
// Returns:
//
//  HRESULT.
//
// Notes:
//
_Check_return_ HRESULT
UtilGetNearestSymbol(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_(MAX_SYMBOL_NAME_LEN) char* buf)
{
	UINT64 disp = 0;
	ULONG cchActual = 0;
	HRESULT hr = hostCtxt->DebugSymbols->GetNameByOffset(
		addr, buf, MAX_SYMBOL_NAME_LEN, &cchActual, &disp);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_NAME_BY_OFFSET,
			addr,
			hr);
		goto exit;
	}

	// If we have a displacement, append it to the name. Don't do it for S_FALSE
	// since that means the symbol was already truncated. No point in appending
	// anything more.
	//
	if (disp && hr != S_FALSE && MAX_SYMBOL_NAME_LEN - cchActual > 0)
	{
		// 'cchActual' includes the NUL, so we want to start writing
		// just before 'cchActual'.
		//
		hr = StringCchPrintfA(
			buf + (cchActual - 1),
			MAX_SYMBOL_NAME_LEN - (cchActual - 1),
			"+%#I64x",
			disp);

		assert(hr != STRSAFE_E_INVALID_PARAMETER);
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilReadWideString
//
// Description:
//
//  Read an optionally counted wide string from the target's memory.
//
// Parameters:
//
//  cchCount - if -1, then assumes string is NUL-terminated, otherwise indicates
//   how many characters to read.
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilReadWideString(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_to_(cchBuf, cchCount) WCHAR* buf,
	_In_ ULONG cchBuf,
	_In_ int cchCount,
	_Out_ ULONG* cchActualLen)
{
	ULONG cbActual = 0;
	HRESULT hr = S_OK;
	if (cchCount < 0)
	{
		// ReadUnicodeStringVirtualWide looks for a NULL-terminator and fails
		// if it doesn't find one within 'maxBytes' bytes.
		//
		hr = hostCtxt->DebugDataSpaces->ReadUnicodeStringVirtualWide(
			addr,
			cchBuf * sizeof(WCHAR),  // maxBytes
			buf,
			cchBuf,
			&cbActual);
	}
	else
	{
		// ReadVirtual just reads arbitrary content up to a given size.
		// We add 1 to cchCount so that the caller gets 'cchCount' non-NUL
		// characters. Otherwise if the caller were to pass '1', we would read
		// no characters because we are telling 'ReadVirtual' our buffer has
		// space only for a NUL.
		//
		hr = hostCtxt->DebugDataSpaces->ReadVirtual(
			addr,
			buf,
			min(cchBuf, (ULONG)cchCount + 1) * sizeof(WCHAR),
			&cbActual);
	}

	if (FAILED(hr))
	{
		goto exit;
	}
	
	*cchActualLen = cbActual / sizeof(WCHAR);
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilReadAnsiString
//
// Description:
//
//  Read an optionally counted ANSI string from the target's memory.
//
// Parameters:
//
//  cchCount - if -1, then assumes string is NUL-terminated, otherwise indicates
//   how many characters to read.
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilReadAnsiString(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_to_(cbBuf, cbCount) char* buf,
	_In_ ULONG cbBuf,
	_In_ int cbCount,
	_Out_ ULONG* cbActualLen)
{
	HRESULT hr = S_OK;
	if (cbCount < 0)
	{
		// ReadMultiByteStringVirtual looks for a NULL-terminator and fails
		// if it doesn't find one within 'maxBytes' bytes.
		//
		hr = hostCtxt->DebugDataSpaces->ReadMultiByteStringVirtual(
			addr,
			cbBuf,  // maxBytes
			buf,
			cbBuf,
			cbActualLen);
	}
	else
	{
		// ReadVirtual just reads arbitrary content up to a given size.
		// We add 1 to cchCount so that the caller gets 'cchCount' non-NUL
		// characters. Otherwise if the caller were to pass '1', we would read
		// no characters because we are telling 'ReadVirtual' our buffer has
		// space only for a NUL.
		//
		hr = hostCtxt->DebugDataSpaces->ReadVirtual(
			addr,
			buf,
			min(cbBuf, (ULONG)cbCount + 1),
			cbActualLen);
	}

	if (FAILED(hr))
	{
		goto exit;
	}
	
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilReadBytes
//
// Description:
//
//  Read bytes from target.
//
// Parameters:
//
//  cbCount - Number of bytes to read.
//  cbActualLen - Actual number of bytes read, which may be smaller than
//  requested size.
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilReadBytes(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_writes_(cbCount) char* buf,
	_In_ ULONG cbCount,
	_Out_ ULONG* cbActualLen)
{
	return hostCtxt->DebugDataSpaces->ReadVirtual(
		addr,
		buf,
		cbCount,
		cbActualLen);
}

//------------------------------------------------------------------------------
// Function: UtilCheckAbort
//
// Description:
//
//  Checks debugger's abort bit.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ bool
UtilCheckAbort(
	_In_ DbgScriptHostContext* hostCtxt)
{
	HRESULT hr = hostCtxt->DebugControl->GetInterrupt();

	// S_OK means user has issued an Ctrl-C or Ctrl-Break.
	//
	return hr == S_OK;
}

//------------------------------------------------------------------------------
// Function: UtilConvertAnsiToWide
//
// Description:
//
//  Helper to convert an ANSI string to wide.
//
// Parameters:
//
// Returns:
//
// Notes:
//
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

//------------------------------------------------------------------------------
// Function: UtilFileExists
//
// Description:
//
//  Helper to determine if a file exists.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ bool
UtilFileExists(
	_In_z_ const WCHAR* wszPath)
{
	const DWORD dwAttrib = GetFileAttributes(wszPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

//------------------------------------------------------------------------------
// Function: UtilFlushMessageBuffer
//
// Description:
//
//  Flush remaining content in the message buffer, if any.
//
// Parameters:
//
// Returns:
//
// Notes:
//
//  Used to avoid WinDbg redrawing the screen too often when flooding it with
//  a lot of output.
//
void
UtilFlushMessageBuffer(
	_In_ DbgScriptHostContext* hostCtxt)
{
	if (hostCtxt->BufPosition > 0)
	{
		// Because dbgeng expects null-terminated strings, terminate the buffer
		// at our current position marker.
		//
		// Last character is always reserved for the NULL terminator.
		// Position must be before that.
		//
		assert(hostCtxt->BufPosition <= _countof(hostCtxt->MessageBuf) - 1);
		hostCtxt->MessageBuf[hostCtxt->BufPosition] = 0;

		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_NORMAL,
			"%s",
			hostCtxt->MessageBuf);

		// Reset position marker.
		//
		hostCtxt->BufPosition = 0;
	}
}

//------------------------------------------------------------------------------
// Function: UtilBufferOutput
//
// Description:
//
//  Buffer 'text' in an internal buffer, flushing when the buffer fills.
//
// Parameters:
//
// Returns:
//
// Notes:
//
//  Used to avoid WinDbg redrawing the screen too often when flooding it with
//  a lot of output.
//
void
UtilBufferOutput(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* text,
	_In_ size_t len)
{
	assert(hostCtxt->IsBuffering > 0);

	// Last byte is reserved for null because windbg only accepts null-terminated
	// strings.
	//
	// If the write would end up trampling our NULL byte, we can't have that.
	// => Flush the current buffer and start anew.
	//
	if (hostCtxt->BufPosition + len > _countof(hostCtxt->MessageBuf) - 1)
	{
		UtilFlushMessageBuffer(hostCtxt);

		assert(hostCtxt->BufPosition == 0);

		// If the string would never fit into the buffer even if it's empty,
		// truncate it.
		//
		// Assert in debug builds.
		//
		assert(len <= _countof(hostCtxt->MessageBuf) - 1);

		if (len > _countof(hostCtxt->MessageBuf) - 1)
		{
			len = _countof(hostCtxt->MessageBuf) - 1;
		}
	}

	// Append the string to our buffer.
	//
	memcpy(hostCtxt->MessageBuf + hostCtxt->BufPosition, text, len);
	hostCtxt->BufPosition += len;
	assert(hostCtxt->BufPosition <= _countof(hostCtxt->MessageBuf) - 1);
}

//------------------------------------------------------------------------------
// Function: UtilExecuteCommand
//
// Description:
//
//  Execute debugger command and output results.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilExecuteCommand(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* command)
{
	HRESULT hr = S_OK;

	// CONSIDER: adding an option letting user control whether we echo the
	// command or not.
	//
	if (hostCtxt->IsBuffering > 0)
	{
		// Capture the output for this scope.
		//
		{
			CAutoSetOutputCallback autoSetCb(
				hostCtxt,
				(IDebugOutputCallbacks*)hostCtxt->BufferedOutputCallbacks);

			hr = autoSetCb.Install();
			if (FAILED(hr))
			{
				hostCtxt->DebugControl->Output(
					DEBUG_OUTPUT_ERROR,
					ERR_FAILED_INSTALL_OUTPUT_CB,
					hr);
				goto exit;
			}

			hr = hostCtxt->DebugControl->Execute(
				DEBUG_OUTCTL_THIS_CLIENT,
				command,
				DEBUG_EXECUTE_NO_REPEAT | DEBUG_OUTCTL_NOT_LOGGED);
			if (FAILED(hr))
			{
				hostCtxt->DebugControl->Output(
					DEBUG_OUTPUT_ERROR,
					ERR_EXEC_CMD_FAILED_FMT,
					command,
					hr);
				goto exit;
			}

			// Dtor will revert the callback.
			//
		}

		// Extract the buffered output.
		//
		const char* buf = DbgScriptOutCallbacksGetBuffer(
			hostCtxt->BufferedOutputCallbacks);
		
		UtilBufferOutput(hostCtxt, buf, strlen(buf));
	}
	else
	{
		hr = hostCtxt->DebugControl->Execute(
			DEBUG_OUTCTL_ALL_CLIENTS,
			command,
			DEBUG_EXECUTE_ECHO | DEBUG_EXECUTE_NO_REPEAT);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_EXEC_CMD_FAILED_FMT,
				command,
				hr);
			goto exit;
		}
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: CAutoSwitchThread ctor
//
// Description:
//
//  Capture thread context to switch to in subsequent call to Switch().
//
// Parameters:
//
// Returns:
//
// Notes:
//
CAutoSwitchThread::CAutoSwitchThread(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const DbgScriptThread* thd)
	:
	m_HostCtxt(hostCtxt),
	m_TargetThreadId(thd->EngineId),
	m_PrevThreadId((ULONG)-1),
	m_DidSwitch(false)
{
}

//------------------------------------------------------------------------------
// Function: CAutoSwitchThread::Switch
//
// Description:
//
//  Switch to captured thread context.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
CAutoSwitchThread::Switch()
{
	// Get current thread id.
	//
	IDebugSystemObjects* sysObj = m_HostCtxt->DebugSysObj;
	HRESULT hr = sysObj->GetCurrentThreadId(&m_PrevThreadId);
	if (FAILED(hr))
	{
		goto exit;
	}

	// Don't bother switching if we're already on the desired thread.
	//
	if (m_PrevThreadId != m_TargetThreadId)
	{
		hr = sysObj->SetCurrentThreadId(m_TargetThreadId);
		if (FAILED(hr))
		{
			goto exit;
		}
		
		m_DidSwitch = true;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: CAutoSwitchThread dtor
//
// Description:
//
//  Revert to previous thread context.
//
// Parameters:
//
// Returns:
//
// Notes:
//
CAutoSwitchThread::~CAutoSwitchThread()
{
	// Revert to previous thread.
	//
	if (m_DidSwitch)
	{
		HRESULT hr = m_HostCtxt->DebugSysObj->SetCurrentThreadId(m_PrevThreadId);
		assert(SUCCEEDED(hr));
		hr;
	}
}

//------------------------------------------------------------------------------
// Function: CAutoSwitchStackFrame ctor
//
// Description:
//
//  Capture stack frame index to switch to in a subsequent call to Switch().
//
// Parameters:
//
// Returns:
//
// Notes:
//
CAutoSwitchStackFrame::CAutoSwitchStackFrame(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG newIdx) 
	:
	m_HostCtxt(hostCtxt),
	m_TargetIdx(newIdx),
	m_PrevIdx((ULONG)-1),
	m_DidSwitch(false)
{
}

//------------------------------------------------------------------------------
// Function: CAutoSwitchStackFrame::Switch
//
// Description:
//
//  Switch to captured stack frame scope.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
CAutoSwitchStackFrame::Switch()
{
	IDebugSymbols3* dbgSymbols = m_HostCtxt->DebugSymbols;
	
	HRESULT hr = dbgSymbols->GetCurrentScopeFrameIndex(&m_PrevIdx);
	if (FAILED(hr))
	{
		m_HostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_SYM_SCOPE,
			hr);
		goto exit;
	}

	if (m_PrevIdx != m_TargetIdx)
	{
		hr = dbgSymbols->SetScopeFrameByIndex(m_TargetIdx);
		if (FAILED(hr))
		{
			m_HostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_FAILED_SET_SYM_SCOPE,
				hr);
			goto exit;
		}
		m_DidSwitch = true;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: CAutoSwitchStackFrame dtor
//
// Description:
//
//  Revert stack frame to previous scope.
//
// Parameters:
//
// Returns:
//
// Notes:
//
CAutoSwitchStackFrame::~CAutoSwitchStackFrame()
{
	if (m_DidSwitch)
	{
		IDebugSymbols3* dbgSymbols = m_HostCtxt->DebugSymbols;
		if (m_PrevIdx != (ULONG)-1)
		{
			HRESULT hr = dbgSymbols->SetScopeFrameByIndex(m_PrevIdx);
			if (FAILED(hr))
			{
				m_HostCtxt->DebugControl->Output(
					DEBUG_OUTPUT_ERROR,
					ERR_FAILED_SET_SYM_SCOPE, hr);
			}
		}
	}
}

//------------------------------------------------------------------------------
// Function: CAutoSetOutputCallback ctor
//
// Description:
//
//  Trivial
//
// Parameters:
//
// Returns:
//
// Notes:
//
CAutoSetOutputCallback::CAutoSetOutputCallback(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ IDebugOutputCallbacks* cb) :
	m_HostCtxt(hostCtxt),
	m_Target(cb),
	m_Prev(nullptr)
{
}

//------------------------------------------------------------------------------
// Function: CAutoSetOutputCallback::Install
//
// Description:
//
//  Install the captured output callbacks.
//
// Parameters:
//
// Returns:
//
//  HRESULT
//
// Notes:
//
_Check_return_ HRESULT
CAutoSetOutputCallback::Install()
{
	IDebugClient* client = m_HostCtxt->DebugClient;
	HRESULT hr = client->GetOutputCallbacks(&m_Prev);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = client->SetOutputCallbacks(m_Target);
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: CAutoSetOutputCallback dtor
//
// Description:
//
//  Revert the installed output callbacks.
//
// Parameters:
//
// Returns:
//
// Notes:
//
CAutoSetOutputCallback::~CAutoSetOutputCallback()
{
	IDebugClient* client = m_HostCtxt->DebugClient;
	HRESULT hr = client->SetOutputCallbacks(m_Prev);
	assert(SUCCEEDED(hr));
	hr;
}


//------------------------------------------------------------------------------
// Function: UtilFindScriptFile
//
// Description:
//
//  Lookup a script file in the registered script paths.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
UtilFindScriptFile(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const WCHAR* scriptName,
	_Out_writes_(cchFullPath) WCHAR* fullPath,
	_In_ int cchFullPath)
{
	HRESULT hr = S_OK;
	
	// First, initialize 'fullPath' with the input string.
	//
	hr = StringCchCopy(fullPath, cchFullPath, scriptName);
	if (FAILED(hr))
	{
		goto exit;
	}
	
	// Try to find the script in the search locations provided by the extension.
	//
	if (!UtilFileExists(fullPath))
	{
		// Try to search for the file in the script path list.
		//
		ScriptPathElem* elem = hostCtxt->ScriptPath;
		while (elem)
		{
			StringCchPrintf(fullPath, cchFullPath, L"%hs\\%ls",
				elem->Path, scriptName);
			if (UtilFileExists(fullPath))
			{
				break;
			}
			elem = elem->Next;
		}
	}

	if (!UtilFileExists(fullPath))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_SCRIPT_NOT_FOUND);
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		goto exit;
	}
	
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilCountStackFrameVariables
//
// Description:
//
//  Helper to enumerate locals or arguments in a stack frame.
//
// Parameters:
//
//  thd - Thread in which stack frame resides.
//
//  stackFrame - Stack frame to iterate.
//
//  flags - Flags passed to GetScopeSymbolGroup2.
//    Either DEBUG_SCOPE_GROUP_LOCALS or DEBUG_SCOPE_GROUP_ARGUMENTS.
//
//  numVars - On return, indicates the number of variables in the stack frame.
//
// Returns:
//
//  HRESULT.
//
// Notes:
//
//  You are expected to call UtilEnumStackFrameVariables after this to free
//  the symbol group returned to you.
//
_Check_return_ HRESULT
UtilCountStackFrameVariables(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const DbgScriptThread* thd,
	_In_ const DbgScriptStackFrame* stackFrame,
	_In_ ULONG flags,
	_Out_ ULONG* numVars,
	_Out_ IDebugSymbolGroup2** symGrp)
{
	IDebugSymbols3* dbgSymbols = hostCtxt->DebugSymbols;
	HRESULT hr = S_OK;

	{
		CAutoSwitchThread autoSwitchThd(hostCtxt, thd);

		hr = autoSwitchThd.Switch();
		if (FAILED(hr))
		{
			goto exit;
		}
		
		CAutoSwitchStackFrame autoSwitchFrame(hostCtxt, stackFrame->FrameNumber);

		hr = autoSwitchFrame.Switch();
		if (FAILED(hr))
		{
			goto exit;
		}
		
		// Take a snapshot of the current symbols in this frame.
		//
		hr = dbgSymbols->GetScopeSymbolGroup2(flags, nullptr, symGrp);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_FAILED_CREATE_SYM_GRP,
				hr);
			goto exit;
		}
	}

	hr = (*symGrp)->GetNumberSymbols(numVars);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_NUM_SYM,
			hr);
		goto exit;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilEnumStackFrameVariables
//
// Description:
//
//  Helper to enumerate locals or arguments in a stack frame.
//
// Parameters:
//
//  symGrp - The symbol group that was returned from 'UtilCountStackFrameVariables'.
//
//  callback - User callback to be called for every variable found.
//
//  userctxt - Optional user context to be passed to the callback.
//
// Returns:
//
//  HRESULT.
//
// Notes:
//
_Check_return_ HRESULT
UtilEnumStackFrameVariables(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ IDebugSymbolGroup2* symGrp,
	_In_ ULONG numSym,
	_In_ EnumStackFrameVarsCb callback,
	_In_opt_ void* userctxt)
{
	assert(symGrp);
	HRESULT hr = S_OK;

	// Iterate the symbols in the group.
	//
	for (ULONG i = 0; i < numSym; ++i)
	{
		char symName[MAX_SYMBOL_NAME_LEN];
		DEBUG_SYMBOL_ENTRY entry = { 0 };

		hr = symGrp->GetSymbolEntryInformation(i, &entry);
		if (hr == E_NOINTERFACE)
		{
			// Sometimes variables are optimized away, which can cause this error.
			// Just leave the size at 0.
			//
		}
		else if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_FAILED_GET_SYM_ENTRY_INFO,
				hr);
			goto exit;
		}

		hr = symGrp->GetSymbolName(i, symName, _countof(symName), nullptr);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_FAILED_GET_SYM_NAME,
				hr);
			goto exit;
		}
		
		// Call the user-supplied callback.
		//
		hr = callback(&entry, symName, i, userctxt);
		if (FAILED(hr))
		{
			goto exit;
		}
	}


exit:
	symGrp->Release();
	symGrp = nullptr;

	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilEnumThreads
//
// Description:
//
//  Count threads in the target process.
//
// Parameters:
//
// Returns:
//
//  HRESULT.
//
// Notes:
//
_Check_return_ HRESULT
UtilCountThreads(
	_In_ DbgScriptHostContext* hostCtxt,
	_Out_ ULONG* cThreads)
{
	IDebugSystemObjects* sysObj = hostCtxt->DebugSysObj;
	HRESULT hr = sysObj->GetNumberThreads(cThreads);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_NUM_THREADS,
			hr);
		goto exit;
	}
exit:
	return hr;
}

//------------------------------------------------------------------------------
// Function: UtilEnumThreads
//
// Description:
//
//  Fetch threads in the target process.
//
// Parameters:
//
// Returns:
//
//  HRESULT.
//
// Notes:
//
_Check_return_ HRESULT
UtilEnumThreads(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG cThreads,
	_Out_writes_(cThreads) ULONG* engineThreadIds,
	_Out_writes_(cThreads) ULONG* sysThreadIds)
{
	IDebugSystemObjects* sysObj = hostCtxt->DebugSysObj;
	HRESULT hr = S_OK;

	// Get list of thread IDs.
	//
	hr = sysObj->GetThreadIdsByIndex(0, cThreads, engineThreadIds, sysThreadIds);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_THREAD_IDS,
			hr);
		goto exit;
	}

exit:
	return hr;
}

