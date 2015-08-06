//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dsstackframe.h
// @Author: alexbud
//
// Purpose:
//
//  DbgScript StackFrame Object.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************
#pragma once

// DbgScriptStackFrame - Object that models a debugger thread.
//
struct DbgScriptStackFrame
{
	// Frame Number.
	//
	ULONG FrameNumber;

	// See DEBUG_STACK_FRAME::InstructionOffset.
	//
	UINT64 InstructionOffset;
};

_Check_return_ HRESULT
DsGetCurrentStackFrame(
	_In_ DbgScriptHostContext* hostCtxt,
	_Out_ DbgScriptStackFrame* stackFrame);

_Check_return_ HRESULT
DsGetStackTrace(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptThread* thread,
	_Out_writes_(cFrames) DEBUG_STACK_FRAME* frames,
	_In_ ULONG cFrames,
	_Out_ ULONG* framesFilled);
