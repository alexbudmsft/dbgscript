//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: stackframe.h
// @Author: alexbud
//
// Purpose:
//
//  StackFrame class for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

struct StackFrameObj
{
	// Language-independent StackFrame.
	//
	DbgScriptStackFrame Frame;

	// Parent thread.
	//
	VALUE Thread;
};

void
Init_StackFrame();
