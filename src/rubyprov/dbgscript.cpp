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

//------------------------------------------------------------------------------
// Function: DbgScript_read_ptr
//
// Synopsis:
//
//  DbgScript.resolve_enum(addr) -> Integer
//
// Description:
//
//  Read a pointer from address 'addr' (Integer).
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

//------------------------------------------------------------------------------
// Function: DbgScript_read_bytes
//
// Synopsis:
//
//  DbgScript.read_bytes(addr, count) -> String
//
// Description:
//
//  Read 'count' bytes from 'addr'.
//
static VALUE
DbgScript_read_bytes(
	_In_ VALUE /* self */,
	_In_ VALUE addr,
	_In_ VALUE count)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	return RbReadBytes(NUM2ULL(addr), count);
}

//------------------------------------------------------------------------------
// Function: DbgScript_read_wide_string
//
// Synopsis:
//
//  DbgScript.read_wide_string(addr [, count]) -> String
//
// Description:
//
//  Read an ANSI string from 'addr'.
//
//  count limits the (int) number of characters to read. -1 means read up to the
//  first NUL.
//
static VALUE
DbgScript_read_wide_string(
	_In_ int argc,
	_In_reads_(argc) VALUE* argv,
	_In_ VALUE /*self*/)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	UINT64 addr = 0;

	// Initialize to default value. -1 means 
	//
	int count = -1;

	if (argc > 2)
	{
		rb_raise(rb_eArgError, "wrong number of arguments");
	}
	else if (argc == 2)
	{
		count = NUM2INT(argv[1]);
	}
	
	addr = NUM2ULL(argv[0]);
	
	return RbReadWideString(addr, count);
}

//------------------------------------------------------------------------------
// Function: DbgScript_read_string
//
// Synopsis:
//
//  DbgScript.read_string(addr [, count]) -> String
//
// Description:
//
//  Read an ANSI string from 'addr'.
//
//  count limits the (int) number of characters to read. -1 means read up to the
//  first NUL.
//
static VALUE
DbgScript_read_string(
	_In_ int argc,
	_In_reads_(argc) VALUE* argv,
	_In_ VALUE /*self*/)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	UINT64 addr = 0;

	// Initialize to default value. -1 means 
	//
	int count = -1;

	if (argc > 2)
	{
		rb_raise(rb_eArgError, "wrong number of arguments");
	}
	else if (argc == 2)
	{
		count = NUM2INT(argv[1]);
	}
	
	addr = NUM2ULL(argv[0]);
	
	return RbReadString(addr, count);
}

//------------------------------------------------------------------------------
// Function: DbgScript_get_peb
//
// Synopsis:
//
//  DbgScript.get_peb -> Integer
//
// Description:
//
//  Get the address of the current process' PEB.
//
static VALUE
DbgScript_get_peb(
	_In_ VALUE /* self */)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	UINT64 addr = 0;
	HRESULT hr = UtilGetPeb(hostCtxt, &addr);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "Failed to get PEB. Error 0x%08x.", hr);
	}

	return ULL2NUM(addr);
}

//------------------------------------------------------------------------------
// Function: DbgScript_field_offset
//
// Synopsis:
// 
//  DbgScript.field_offset(type, field) -> Integer
//
// Description:
//
//  Return the offset of 'field' in 'type'.
//
static VALUE
DbgScript_field_offset(
	_In_ VALUE /* self */,
	_In_ VALUE type,
	_In_ VALUE field)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	ULONG offset = 0;
	HRESULT hr = UtilGetFieldOffset(
		hostCtxt, StringValuePtr(type), StringValuePtr(field), &offset);
	
	if (FAILED(hr))
	{
		rb_raise(
			rb_eArgError,
			"Failed to get field offset for type '%s' and field '%s'. Error 0x%08x.",
			type,
			field,
			hr);
	}

	return ULONG2NUM(offset);
}

//------------------------------------------------------------------------------
// Function: DbgScript_get_type_size
//
// Synopsis:
// 
//  DbgScript.get_type_size(type) -> Integer
//
// Description:
//
//  Return the size of 'type' in bytes.
//
static VALUE
DbgScript_get_type_size(
	_In_ VALUE /* self */,
	_In_ VALUE type)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	ULONG size = 0;
	HRESULT hr = UtilGetTypeSize(hostCtxt, StringValuePtr(type), &size);
	
	if (FAILED(hr))
	{
		rb_raise(
			rb_eArgError,
			"Failed to get size of type '%s'. Error 0x%08x.",
			type,
			hr);
	}

	return ULONG2NUM(size);
}

//------------------------------------------------------------------------------
// Function: DbgScript_get_nearest_sym
//
// Synopsis:
//
//  DbgScript.get_nearest_sym(addr) -> String
//
// Description:
//
//  Lookup nearest symbol to address 'addr' (Integer).
//
static VALUE
DbgScript_get_nearest_sym(
	_In_ VALUE /* self */,
	_In_ VALUE addr)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	const UINT64 ui64Addr = NUM2ULL(addr);
	char name[MAX_SYMBOL_NAME_LEN] = {};

	HRESULT hr = UtilGetNearestSymbol(hostCtxt, ui64Addr, name);
	if (FAILED(hr))
	{
		rb_raise(rb_eArgError, "Failed to resolve symbol from address %p. Error 0x%08x.", addr, hr);
	}

	return rb_str_new2(name);
}

//------------------------------------------------------------------------------
// Function: DbgScript_resolve_enum
//
// Synopsis:
//
//  DbgScript.resolve_enum(type, value) -> String
//
// Description:
//
//  Resolve an enum value to its symbolic identifier (String). 'type' identifies
//  the enum type and 'value' is the value to resolve.
//
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

//------------------------------------------------------------------------------
// Function: DbgScript_get_global
//
// Synopsis:
//
//  DbgScript.get_global(sym) -> TypedObject
//
// Description:
//
//  Get a global variable as a TypedObject. 'sym' identifies the global
//  symbol to retrieve.
//
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
// Synopsis:
//
//  DbgScript.current_thread -> Thread
//
// Description:
//
//  Get the current thread.
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
// Synopsis:
//
//  DbgScript.get_threads() -> Array of Thread
//
// Description:
//
//  Get all threads in the process.
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

//------------------------------------------------------------------------------
// Function: DbgScript_start_buffering
//
// Synopsis:
//
//  DbgScript.start_buffering() -> nil
//
// Description:
//
//  Start an output buffering session. This can improve performance when writing
//  a lot of content out to the debugger. dbgeng will normally flush every line
//  of output which can cause undue performance degradation. Starting a buffering
//  session will buffer the output in 8K chunks and flush them as they get filled.
//
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

//------------------------------------------------------------------------------
// Function: DbgScript_stop_buffering
//
// Synopsis:
//
//  DbgScript.stop_buffering() -> nil
//
// Description:
//
//  Stop an output buffering session, and flushes any remaining buffer.
//
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

//------------------------------------------------------------------------------
// Function: DbgScript_execute_command
//
// Synopsis:
//
//  DbgScript.execute_command(command) -> nil
//
// Description:
//
//  Execute a dbgeng command and output the results.
//
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

//------------------------------------------------------------------------------
// Function: DbgScript_create_typed_object
//
// Synopsis:
//
//  DbgScript.create_typed_object(type, address) -> TypedObject
//
// Description:
//
//  Create a TypedObject from a type (String) and address (Integer).
//
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
		module, "read_bytes", RUBY_METHOD_FUNC(DbgScript_read_bytes), 2 /* argc */);
	
	rb_define_module_function(
		module, "read_string", RUBY_METHOD_FUNC(DbgScript_read_string), -1 /* argc */);
	
	rb_define_module_function(
		module, "read_wide_string", RUBY_METHOD_FUNC(DbgScript_read_wide_string), -1 /* argc */);
	
	rb_define_module_function(
		module, "get_peb", RUBY_METHOD_FUNC(DbgScript_get_peb), 0 /* argc */);
	
	rb_define_module_function(
		module, "field_offset", RUBY_METHOD_FUNC(DbgScript_field_offset), 2 /* argc */);
	
	rb_define_module_function(
		module, "get_type_size", RUBY_METHOD_FUNC(DbgScript_get_type_size), 1 /* argc */);
	
	rb_define_module_function(
		module, "get_nearest_sym", RUBY_METHOD_FUNC(DbgScript_get_nearest_sym), 1 /* argc */);
	
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

