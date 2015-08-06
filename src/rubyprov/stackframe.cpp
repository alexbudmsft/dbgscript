//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: stackframe.cpp
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
#include "common.h"
#include "stackframe.h"

//------------------------------------------------------------------------------
// Function: StackFrame_free
//
// Description:
//
//  Frees a DbgScriptStackFrame object allocated by 'StackFrame_alloc'.
//  
// Returns:
//
// Notes:
//
static void
StackFrame_free(
	_In_ void* obj)
{
	DbgScriptStackFrame* frame = (DbgScriptStackFrame*)obj;
	delete frame;
}

//------------------------------------------------------------------------------
// Function: StackFrame_mark
//
// Description:
//
//  Marks children of a DbgScriptStackFrame object to prevent their premature
//  garbage collection. (Called during 'mark' phase of mark-and-sweep Ruby GC.)
//  
// Returns:
//
// Notes:
//
static void
StackFrame_mark(
	_In_ void* obj)
{
	StackFrameObj* frame = (StackFrameObj*)obj;
	rb_gc_mark(frame->Thread);
}

//------------------------------------------------------------------------------
// Function: StackFrame_alloc
//
// Description:
//
//  Allocates a Ruby-wrapped DbgScriptStackFrame object.
//  
// Returns:
//
// Notes:
//
static VALUE
StackFrame_alloc(
	_In_ VALUE klass)
{
	StackFrameObj* frame = new StackFrameObj;
	memset(frame, 0, sizeof(*frame));

	return Data_Wrap_Struct(klass, StackFrame_mark, StackFrame_free, frame);
}

void
Init_StackFrame()
{
	VALUE stackFrameClass = rb_define_class_under(
		GetRubyProvGlobals()->DbgScriptModule,
		"StackFrame",
		rb_cObject);

	rb_define_alloc_func(stackFrameClass, StackFrame_alloc);
	
	// Prevent scripter from instantiating directly.
	//
	LockDownClass(stackFrameClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->StackFrameClass = stackFrameClass;
}
