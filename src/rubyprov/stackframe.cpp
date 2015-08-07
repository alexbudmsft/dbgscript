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
// Function: StackFrame_frame_number
//
// Description:
//
//  Attribute-style getter method for the StackFrame's frame number.
//  
// Returns:
//
// Notes:
//
static VALUE
StackFrame_frame_number(
	_In_ VALUE self)
{
	StackFrameObj* frame = nullptr;

	Data_Get_Struct(self, StackFrameObj, frame);

	return ULONG2NUM(frame->Frame.FrameNumber);
}

//------------------------------------------------------------------------------
// Function: StackFrame_frame_number
//
// Description:
//
//  Attribute-style getter method for the StackFrame's instruction offset.
//  
// Returns:
//
// Notes:
//
static VALUE
StackFrame_instruction_offset(
	_In_ VALUE self)
{
	StackFrameObj* frame = nullptr;

	Data_Get_Struct(self, StackFrameObj, frame);

	return ULL2NUM(frame->Frame.InstructionOffset);
}

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
	StackFrameObj* frame = (StackFrameObj*)obj;
	delete frame;
}

//------------------------------------------------------------------------------
// Function: StackFrame_mark
//
// Description:
//
//  Marks children of a StackFrameObj object to prevent their premature
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
//  Allocates a Ruby-wrapped StackFrameObj object.
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
	
	rb_define_method(
		stackFrameClass,
		"frame_number",
		RUBY_METHOD_FUNC(StackFrame_frame_number),
		0 /* argc */);
	
	rb_define_method(
		stackFrameClass,
		"instruction_offset",
		RUBY_METHOD_FUNC(StackFrame_instruction_offset),
		0 /* argc */);
	
	// Prevent scripter from instantiating directly.
	//
	LockDownClass(stackFrameClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->StackFrameClass = stackFrameClass;
}
