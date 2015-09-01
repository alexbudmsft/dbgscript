//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dsthread.cpp
// @Author: alexbud
//
// Purpose:
//
//  Implements support routines for DbgScriptThread.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "../common.h"
#include "util.h"

//------------------------------------------------------------------------------
// Function: DsThreadGetTeb
//
// Description:
//
//  Get the TEB (Thread Environment Block) address for a given thread.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
DsThreadGetTeb(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptThread* thread,
	_Out_ UINT64* tebAddr)
{
	HRESULT hr = S_OK;
	
	{
		// Need to switch thread context to 'thread', capture stack, then
		// switch back.
		//
		CAutoSwitchThread autoSwitchThd(hostCtxt, thread);
		
		hr = autoSwitchThd.Switch();
		if (FAILED(hr))
		{
			goto exit;
		}
		
		// Get TEB from debug client.
		//
		hr = hostCtxt->DebugSysObj->GetCurrentThreadTeb(tebAddr);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				ERR_FAILED_GET_TEB,
				hr);
			goto exit;
		}
	}
exit:
	return hr;
}
