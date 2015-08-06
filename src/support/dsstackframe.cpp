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
