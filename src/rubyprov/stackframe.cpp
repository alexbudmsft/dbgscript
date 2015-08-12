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
#include "typedobject.h"

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
// Function: buildArrayFromLocals
//
// Description:
//
//  Callback to UtilEnumStackFrameVariables to build a Ruby array containing
//  the locals in the stack frame.
//  
// Returns:
//
// Notes:
//
static _Check_return_ HRESULT
buildArrayFromLocals(
	_In_ DEBUG_SYMBOL_ENTRY* entry,
	_In_z_ const char* symName,
	_In_z_ const char* typeName,
	_In_ ULONG idx,
	_In_opt_ void* ctxt)
{
	VALUE stackArray = (VALUE)ctxt;

	VALUE localVar = AllocTypedObject(
		entry->Size,
		symName,
		typeName,
		entry->TypeId,
		entry->ModuleBase,
		entry->Offset);

	rb_ary_store(stackArray, idx, localVar);
	
	return S_OK;
}

//------------------------------------------------------------------------------
// Function: getVariablesHelper
//
// Description:
//
//  Helper to fetch locals/args from the given stack frame.
//  
// Returns:
//
//  A Ruby array containing the locals or args in the frame.
//
// Notes:
//
static VALUE
getVariablesHelper(
	_In_ StackFrameObj* stackFrame,
	_In_ ULONG flags)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	HRESULT hr = S_OK;
	ULONG numSym = 0;
	IDebugSymbolGroup2* symGrp = nullptr;
	VALUE stackArray = 0;

	DbgScriptThread* thd = nullptr;

	Data_Get_Struct(stackFrame->Thread, DbgScriptThread, thd);

	hr = UtilCountStackFrameVariables(
		hostCtxt,
		thd,
		&stackFrame->Frame,
		flags,
		&numSym,
		&symGrp);
	if (FAILED(hr))
	{
		rb_raise(rb_eSystemCallError, "UtilCountStackFrameVariables failed. Error: 0x%08x", hr);
	}
	
	stackArray = rb_ary_new2(numSym);
	
	hr = UtilEnumStackFrameVariables(
		hostCtxt,
		symGrp,
		numSym,
		buildArrayFromLocals,
		(void*)stackArray);
	if (FAILED(hr))
	{
		rb_raise(rb_eSystemCallError, "UtilEnumStackFrameVariables failed. Error: 0x%08x", hr);
	}
	
	return stackArray;
}

//------------------------------------------------------------------------------
// Function: StackFrame_get_locals
//
// Description:
//
//  Get local variables in 'self'. This includes arguments.
//  
// Returns:
//
// Notes:
//
static VALUE
StackFrame_get_locals(
	_In_ VALUE self)
{
	StackFrameObj* frame = nullptr;

	Data_Get_Struct(self, StackFrameObj, frame);
	return getVariablesHelper(frame, DEBUG_SCOPE_GROUP_LOCALS);
}

//------------------------------------------------------------------------------
// Function: StackFrame_get_args
//
// Description:
//
//  Get arguments in 'self'.
//  
// Returns:
//
// Notes:
//
static VALUE
StackFrame_get_args(
	_In_ VALUE self)
{
	StackFrameObj* frame = nullptr;

	Data_Get_Struct(self, StackFrameObj, frame);
	return getVariablesHelper(frame, DEBUG_SCOPE_GROUP_ARGUMENTS);
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
	
	rb_define_method(
		stackFrameClass,
		"get_locals",
		RUBY_METHOD_FUNC(StackFrame_get_locals),
		0 /* argc */);

	rb_define_method(
		stackFrameClass,
		"get_args",
		RUBY_METHOD_FUNC(StackFrame_get_args),
		0 /* argc */);
	
	// Prevent scripter from instantiating directly.
	//
	LockDownClass(stackFrameClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->StackFrameClass = stackFrameClass;
}
