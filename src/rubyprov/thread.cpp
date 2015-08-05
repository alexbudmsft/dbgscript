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

//------------------------------------------------------------------------------
// Function: Thread_engine_id
//
// Description:
//
//  Attribute-style getter method for the thread's engine id.
//  
// Returns:
//
// Notes:
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
// Function: Thread_free
//
// Description:
//
//  Frees a DbgScriptThread object allocated by 'Thread_alloc'.
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
//  Allocates a Ruby-wrapped DbgScriptThread object.
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
		(RUBYMETHOD)Thread_engine_id,
		0 /* argc */);

	rb_define_alloc_func(threadClass, Thread_alloc);
}

