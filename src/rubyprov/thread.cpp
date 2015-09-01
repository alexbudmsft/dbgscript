//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: thread.cpp
// @Author: alexbud
//
// Purpose:
//
//  Thread class for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#include "common.h"
#include "stackframe.h"

//------------------------------------------------------------------------------
// Function: Thread_engine_id
//
// Synopsis:
//
//  obj.engine_id -> Integer
//
// Description:
//
//  Return the thread's engine id. This is a dbgeng-assigned value for the
//  thread. Not to be confused with the [system] thread id.
//
//
static VALUE
Thread_engine_id(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	DbgScriptThread* thd = nullptr;

	Data_Get_Struct(self, DbgScriptThread, thd);

	return ULONG2NUM(thd->EngineId);
}

//------------------------------------------------------------------------------
// Function: Thread_thread_id
//
// Synopsis:
//
//  obj.thread_id -> Integer
//
// Description:
//
//  Return the thread's system thread id.
//
static VALUE
Thread_thread_id(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	DbgScriptThread* thd = nullptr;

	Data_Get_Struct(self, DbgScriptThread, thd);

	return ULONG2NUM(thd->ThreadId);
}

//------------------------------------------------------------------------------
// Function: Thread_get_stack
//
// Synopsis:
//
//  obj.get_stack -> array of StackFrame
//
// Description:
//
//  Return the call stack as an array of StackFrame objects.
//
static VALUE
Thread_get_stack(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	DEBUG_STACK_FRAME frames[512];
	ULONG framesFilled = 0;
	
	DbgScriptThread* thd = nullptr;
	Data_Get_Struct(self, DbgScriptThread, thd);
	
	// Call the support library to fill in the frame information.
	//
	HRESULT hr = DsGetStackTrace(
		hostCtxt,
		thd,
		frames,
		_countof(frames),
		&framesFilled);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "DsGetStackTrace failed. Error 0x%08x.", hr);
	}

	VALUE framesArray = rb_ary_new2(framesFilled);

	// Build an array of StackFrame objects.
	//
	for (ULONG i = 0; i < framesFilled; ++i)
	{
		// Allocate a StackFrame object.
		//
		VALUE frameObj = rb_class_new_instance(
			0, nullptr, GetRubyProvGlobals()->StackFrameClass);
		
		StackFrameObj* frame = nullptr;
		
		Data_Get_Struct(frameObj, StackFrameObj, frame);
		
		// Copy over the fields.
		//
		frame->Frame.FrameNumber = frames[i].FrameNumber;
		frame->Frame.InstructionOffset = frames[i].InstructionOffset;
		frame->Thread = self;

		rb_ary_store(framesArray, i, frameObj);
	}

	return framesArray;
}

//------------------------------------------------------------------------------
// Function: Thread_get_teb
//
// Synopsis:
//
//  obj.teb -> Integer
//
// Description:
//
//  Return the current stack frame.
//
static VALUE
Thread_get_teb(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptThread* thd = nullptr;
	Data_Get_Struct(self, DbgScriptThread, thd);
	
	// Get TEB from debug client.
	//
	UINT64 teb = 0;
	HRESULT hr = DsThreadGetTeb(hostCtxt, thd, &teb);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "DsThreadGetTeb failed. Error 0x%08x.", hr);
	}

	return ULL2NUM(teb);
}

//------------------------------------------------------------------------------
// Function: Thread_current_frame
//
// Synopsis:
//
//  obj.current_frame -> StackFrame
//
// Description:
//
//  Return the current stack frame.
//
static VALUE
Thread_current_frame(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptThread* thd = nullptr;
	Data_Get_Struct(self, DbgScriptThread, thd);
	
	DbgScriptStackFrame dsframe = {};

	// Call the support library to fill in the frame information.
	//
	HRESULT hr = DsGetCurrentStackFrame(hostCtxt, thd, &dsframe);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "DsGetCurrentStackFrame failed. Error 0x%08x.", hr);
	}

	// Allocate a StackFrame object.
	//
	VALUE frameObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->StackFrameClass);

	StackFrameObj* frame = nullptr;

	Data_Get_Struct(frameObj, StackFrameObj, frame);

	// Copy over the fields.
	//
	frame->Frame = dsframe;
	
	frame->Thread = self;
	
	return frameObj;
}

//------------------------------------------------------------------------------
// Function: Thread_free
//
// Description:
//
//  Free routine for Thread class.
//  
// Returns:
//
// Notes:
//
static void
Thread_free(
	_In_ void* obj)
{
	DbgScriptThread* thd = (DbgScriptThread*)obj;
	delete thd;
}

//------------------------------------------------------------------------------
// Function: Thread_alloc
//
// Description:
//
//  Allocator routine for Thread class.
//  
// Returns:
//
// Notes:
//
static VALUE
Thread_alloc(
	_In_ VALUE klass)
{
	DbgScriptThread* thd = new DbgScriptThread;
	memset(thd, 0, sizeof(*thd));

	return Data_Wrap_Struct(klass, nullptr /* mark */, Thread_free, thd);
}

//------------------------------------------------------------------------------
// Function: AllocThreadObj
//
// Description:
//
//  Allocate and initialize a Thread object.
//  
// Returns:
//
// Notes:
//
_Check_return_ VALUE
AllocThreadObj(
	_In_ ULONG engineId,
	_In_ ULONG threadId)
{
	// Calls allocator routine (Thread_alloc).
	//
	VALUE thdObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->ThreadClass);

	DbgScriptThread* thd = nullptr;

	Data_Get_Struct(thdObj, DbgScriptThread, thd);

	thd->EngineId = engineId;
	thd->ThreadId = threadId;

	return thdObj;
}

//------------------------------------------------------------------------------
// Function: Init_Thread
//
// Description:
//
//  Initializes the Thread class.
//  
// Returns:
//
// Notes:
//
void
Init_Thread()
{
	VALUE threadClass = rb_define_class_under(
		GetRubyProvGlobals()->DbgScriptModule,
		"Thread",
		rb_cObject);

	rb_define_method(
		threadClass,
		"engine_id",
		RUBY_METHOD_FUNC(Thread_engine_id),
		0 /* argc */);

	rb_define_method(
		threadClass,
		"thread_id",
		RUBY_METHOD_FUNC(Thread_thread_id),
		0 /* argc */);

	rb_define_method(
		threadClass,
		"current_frame",
		RUBY_METHOD_FUNC(Thread_current_frame),
		0 /* argc */);
	
	rb_define_method(
		threadClass,
		"get_stack",
		RUBY_METHOD_FUNC(Thread_get_stack),
		0 /* argc */);
	
	rb_define_method(
		threadClass,
		"teb",
		RUBY_METHOD_FUNC(Thread_get_teb),
		0 /* argc */);
	
	rb_define_alloc_func(threadClass, Thread_alloc);

	// Prevent scripter from instantiating directly.
	//
	LockDownClass(threadClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->ThreadClass = threadClass;
}
