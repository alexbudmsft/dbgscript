//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dsstackframe.cpp
// @Author: alexbud
//
// Purpose:
//
//  Implements support routines for DbgScriptStackFrame.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "../common.h"
#include "util.h"

_Check_return_ HRESULT
DsGetCurrentStackFrame(
	_In_ DbgScriptHostContext* hostCtxt,
	_Out_ DbgScriptStackFrame* stackFrame)
{
	IDebugSymbols3* dbgSym = hostCtxt->DebugSymbols;
	ULONG curFrameIdx = 0;
	UINT64 instructionOffset = 0;
	DEBUG_STACK_FRAME frame;

	HRESULT hr = dbgSym->GetCurrentScopeFrameIndex(&curFrameIdx);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to get current frame index. Error 0x%08x.\n", hr);
		goto exit;
	}

	hr = dbgSym->GetScope(&instructionOffset, &frame, nullptr, 0);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to get current scope. Error 0x%08x.\n", hr);
		goto exit;
	}
	
	stackFrame->FrameNumber = curFrameIdx;
	stackFrame->InstructionOffset = instructionOffset;

exit:
	return hr;
}

_Check_return_ HRESULT
DsGetStackTrace(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptThread* thread,
	_Out_writes_(cFrames) DEBUG_STACK_FRAME* frames,
	_In_ ULONG cFrames,
	_Out_ ULONG* framesFilled)
{
	HRESULT hr = S_OK;
	
	{
		// Need to switch thread context to 'thread', capture stack, then
		// switch back.
		//
		CAutoSwitchThread autoSwitchThd(hostCtxt, thread);

		hr = hostCtxt->DebugControl->GetStackTrace(
			0, 0, 0, frames, cFrames, framesFilled);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: Failed to get stacktrace. Error 0x%08x.\n", hr);
			goto exit;
		}

		assert(*framesFilled > 0);
	}
exit:
	return hr;
}

