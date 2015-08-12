#include "util.h"
#include <assert.h>
#include <strsafe.h>

// Read a pointer from the target's memory.
//
_Check_return_ HRESULT
UtilReadPointer(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 addr,
	_Out_ UINT64* ptrVal)
{
	return hostCtxt->DebugDataSpaces->ReadPointersVirtual(1, addr, ptrVal);
}

// Checks debugger's abort bit.
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

// Buffer the output if it fits, else flush the cache.
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

			hr = hostCtxt->DebugControl->Execute(
				DEBUG_OUTCTL_THIS_CLIENT,
				command,
				DEBUG_EXECUTE_NO_REPEAT | DEBUG_OUTCTL_NOT_LOGGED);
			if (FAILED(hr))
			{
				hostCtxt->DebugControl->Output(
					DEBUG_OUTPUT_ERROR,
					ERR_EXEC_CMD_FAILED_FMT, command, hr);
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
				ERR_EXEC_CMD_FAILED_FMT, command, hr);
			goto exit;
		}
	}
exit:
	return hr;
}

CAutoSwitchThread::CAutoSwitchThread(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const DbgScriptThread* thd)
	:
	m_HostCtxt(hostCtxt),
	m_DidSwitch(false)
{
	// Get current thread id.
	//
	IDebugSystemObjects* sysObj = hostCtxt->DebugSysObj;
	HRESULT hr = sysObj->GetCurrentThreadId(&m_PrevThreadId);
	assert(SUCCEEDED(hr));

	// Don't bother switching if we're already on the desired thread.
	//
	if (m_PrevThreadId != thd->EngineId)
	{
		hr = sysObj->SetCurrentThreadId(thd->EngineId);
		assert(SUCCEEDED(hr));
		m_DidSwitch = true;
	}
}

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

CAutoSwitchStackFrame::CAutoSwitchStackFrame(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG newIdx) :
	m_HostCtxt(hostCtxt),
	m_PrevIdx((ULONG)-1),
	m_DidSwitch(false)
{
	IDebugSymbols3* dbgSymbols = hostCtxt->DebugSymbols;
	
	HRESULT hr = dbgSymbols->GetCurrentScopeFrameIndex(&m_PrevIdx);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_SYM_SCOPE, hr);
		goto exit;
	}

	if (m_PrevIdx != newIdx)
	{
		hr = dbgSymbols->SetScopeFrameByIndex(newIdx);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_FAILED_SET_SYM_SCOPE, hr);
			goto exit;
		}
		m_DidSwitch = true;
	}
exit:
	;
}

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

CAutoSetOutputCallback::CAutoSetOutputCallback(
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
	
CAutoSetOutputCallback::~CAutoSetOutputCallback()
{
	IDebugClient* client = m_HostCtxt->DebugClient;
	HRESULT hr = client->SetOutputCallbacks(m_Prev);
	assert(SUCCEEDED(hr));
	hr;
}

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
			"Error: Script file not found in any of the search paths.\n");
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		goto exit;
	}
	
exit:
	return hr;
}
