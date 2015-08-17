//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dbgscript.cpp
// @Author: alexbud
//
// Purpose:
//
//  DbgScript module for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "common.h"
#include "typedobject.h"
#include "thread.h"

// Read a pointer from address 'addr'.
//
static VALUE
DbgScript_read_ptr(
	_In_ VALUE /* self */,
	_In_ VALUE addr)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	const UINT64 ui64Addr = NUM2ULL(addr);
	UINT64 ptrVal = 0;

	HRESULT hr = UtilReadPointer(hostCtxt, ui64Addr, &ptrVal);
	if (FAILED(hr))
	{
		rb_raise(rb_eArgError, "Failed to read pointer value from address '%p'. Error 0x%08x.", addr, hr);
	}

	return ULL2NUM(ptrVal);
}

static VALUE
DbgScript_resolve_enum(
	_In_ VALUE /* self */,
	_In_ VALUE type,
	_In_ VALUE val)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	const char* enumTypeName = StringValuePtr(type);
	UINT64 value = NUM2ULL(val);
	char enumElementName[MAX_SYMBOL_NAME_LEN] = {};

	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, enumTypeName);
	if (!typeInfo)
	{
		rb_raise(rb_eArgError, "Failed to get type id for type '%s'.", enumTypeName);
	}

	HRESULT hr = hostCtxt->DebugSymbols->GetConstantName(
		typeInfo->ModuleBase,
		typeInfo->TypeId,
		value,
		STRING_AND_CCH(enumElementName),
		nullptr);
	if (FAILED(hr))
	{
		rb_raise(rb_eArgError, "Failed to get element name for enum '%s' with value '%llu'. Error 0x%08x.", enumTypeName, value, hr);
	}

	return rb_str_new2(enumElementName);
}

static VALUE
DbgScript_get_global(
	_In_ VALUE /* self */,
	_In_ VALUE sym)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	const char* symbol = StringValuePtr(sym);
	UINT64 addr = 0;
	HRESULT hr = hostCtxt->DebugSymbols->GetOffsetByName(symbol, &addr);
	if (FAILED(hr))
	{
		rb_raise(rb_eArgError, "Failed to get virtual address for symbol '%s'. Error 0x%08x.", symbol, hr);
	}

	ModuleAndTypeId* typeInfo = GetCachedSymbolType(
		hostCtxt, symbol);
	if (!typeInfo)
	{
		rb_raise(rb_eArgError, "Failed to get type id for type '%s'.", symbol);
	}

	return AllocTypedObject(
		0 /* size */,
		symbol /* name */,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		addr);
}

//------------------------------------------------------------------------------
// Function: DbgScript_current_thread
//
// Description:
//
//  Get current thread in the process.
//  
// Returns:
//
//  Thread object as Ruby VALUE.
//
// Notes:
//
static VALUE
DbgScript_current_thread(
	_In_ VALUE /* self */)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	// Get TID from debug client.
	//
	ULONG engineThreadId = 0;
	ULONG systemThreadId = 0;
	HRESULT hr = hostCtxt->DebugSysObj->GetCurrentThreadId(&engineThreadId);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "Failed to get engine thread id. Error 0x%08x.", hr);
	}

	hr = hostCtxt->DebugSysObj->GetCurrentThreadSystemId(&systemThreadId);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "Failed to get system thread id. Error 0x%08x.", hr);
	}

	return AllocThreadObj(engineThreadId, systemThreadId);
}

//------------------------------------------------------------------------------
// Function: DbgScript_get_threads
//
// Description:
//
//  Get all threads in the process as a Ruby array.
//  
// Returns:
//
//  Array of Thread objects.
//
// Notes:
//
static VALUE
DbgScript_get_threads(
	_In_ VALUE /* self */)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	ULONG cThreads = 0;
	HRESULT hr = UtilCountThreads(hostCtxt, &cThreads);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilCountThreads failed. Error 0x%08x.", hr);
	}

	// Get list of thread IDs.
	//
	ULONG* engineThreadIds = new ULONG[cThreads];
	ULONG* sysThreadIds = new ULONG[cThreads];

	hr = UtilEnumThreads(hostCtxt, cThreads, engineThreadIds, sysThreadIds);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilEnumThreads failed. Error 0x%08x.", hr);
	}

	VALUE threadArray = rb_ary_new2(cThreads);

	// Build a tuple of Thread objects.
	//
	for (ULONG i = 0; i < cThreads; ++i)
	{
		VALUE thd = AllocThreadObj(engineThreadIds[i], sysThreadIds[i]);
		rb_ary_store(threadArray, i, thd);
	}

	// Deleting nullptr is safe.
	//
	delete[] engineThreadIds;
	delete[] sysThreadIds;

	return threadArray;
}

static VALUE
DbgScript_start_buffering(
	_In_ VALUE /* self */)
{
	// Currently this is single-threaded access, but will make it simpler in
	// case we ever have more than one concurrent client.
	//
	InterlockedIncrement(&GetRubyProvGlobals()->HostCtxt->IsBuffering);

	return Qnil;
}

static VALUE
DbgScript_stop_buffering(
	_In_ VALUE /* self */)
{
	// Currently this is single-threaded access, but will make it simpler in
	// case we ever have more than one concurrent client.
	//
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const LONG newVal = InterlockedDecrement(&hostCtxt->IsBuffering);
	if (newVal < 0)
	{
		hostCtxt->IsBuffering = 0;
		rb_raise(rb_eRuntimeError, "Can't stop buffering if it isn't started.");
	}

	// If the buffer refcount hit zero, flush remaining buffered content, if any.
	//
	if (newVal == 0)
	{
		UtilFlushMessageBuffer(hostCtxt);
	}

	return Qnil;
}

static VALUE
DbgScript_execute_command(
	_In_ VALUE /* self */,
	_In_ VALUE cmd)
{
	const char* command = StringValuePtr(cmd);
	HRESULT hr = S_OK;

	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	hr = UtilExecuteCommand(hostCtxt, command);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilExecuteCommand failed. Error 0x%08x.", hr);
	}

	return Qnil;
}

static VALUE
DbgScript_create_typed_object(
	_In_ VALUE /* self */,
	_In_ VALUE type,
	_In_ VALUE addr)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	const char* szType = StringValuePtr(type);
	const UINT64 ui64Addr = NUM2ULL(addr);

	// Lookup typeid/moduleBase from type name.
	//
	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, szType);
	if (!typeInfo)
	{
		rb_raise(rb_eArgError, "Failed to get type id for type '%s'.", szType);
	}

	return AllocTypedObject(
		0 /* size */,
		nullptr /* name */,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		ui64Addr);
}

void
Init_DbgScript()
{
	VALUE module = rb_define_module("DbgScript");

	rb_define_module_function(
		module, "read_ptr", RUBY_METHOD_FUNC(DbgScript_read_ptr), 1 /* argc */);

	rb_define_module_function(
		module, "current_thread", RUBY_METHOD_FUNC(DbgScript_current_thread), 0 /* argc */);
	
	rb_define_module_function(
		module, "get_threads", RUBY_METHOD_FUNC(DbgScript_get_threads), 0 /* argc */);
	
	rb_define_module_function(
		module, "create_typed_object", RUBY_METHOD_FUNC(DbgScript_create_typed_object), 2 /* argc */);
	
	rb_define_module_function(
		module, "resolve_enum", RUBY_METHOD_FUNC(DbgScript_resolve_enum), 2 /* argc */);
	
	rb_define_module_function(
		module, "get_global", RUBY_METHOD_FUNC(DbgScript_get_global), 1 /* argc */);
	
	rb_define_module_function(
		module, "start_buffering", RUBY_METHOD_FUNC(DbgScript_start_buffering), 0 /* argc */);

	rb_define_module_function(
		module, "stop_buffering", RUBY_METHOD_FUNC(DbgScript_stop_buffering), 0 /* argc */);
	
	rb_define_module_function(
		module, "execute_command", RUBY_METHOD_FUNC(DbgScript_execute_command), 1 /* argc */);
	
	// Save off the module.
	//
	GetRubyProvGlobals()->DbgScriptModule = module;
}

