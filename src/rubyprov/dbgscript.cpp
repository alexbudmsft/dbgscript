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
	char typeName[MAX_SYMBOL_NAME_LEN] = {};
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

	hr = hostCtxt->DebugSymbols->GetTypeName(
		typeInfo->ModuleBase,
		typeInfo->TypeId,
		STRING_AND_CCH(typeName),
		nullptr);
	if (FAILED(hr))
	{
		rb_raise(rb_eArgError, "Failed to get type name for symbol '%s'. Error 0x%08x.", symbol, hr);
	}
	
	return AllocTypedObject(
		0 /* size */,
		symbol /* name */,
		typeName,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		addr);
}

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
		rb_raise(rb_eSystemCallError, "Failed to get engine thread id. Error 0x%08x.", hr);
	}

	hr = hostCtxt->DebugSysObj->GetCurrentThreadSystemId(&systemThreadId);
	if (FAILED(hr))
	{
		rb_raise(rb_eSystemCallError, "Failed to get system thread id. Error 0x%08x.", hr);
	}

	// Allocate a Thread object.
	//
	VALUE thdObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->ThreadClass);

	DbgScriptThread* thd = nullptr;

	Data_Get_Struct(thdObj, DbgScriptThread, thd);

	thd->EngineId = engineThreadId;
	thd->ThreadId = systemThreadId;

	return thdObj;
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
		szType,
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
		module, "create_typed_object", RUBY_METHOD_FUNC(DbgScript_create_typed_object), 2 /* argc */);
	
	rb_define_module_function(
		module, "resolve_enum", RUBY_METHOD_FUNC(DbgScript_resolve_enum), 2 /* argc */);
	
	rb_define_module_function(
		module, "get_global", RUBY_METHOD_FUNC(DbgScript_get_global), 1 /* argc */);
	
	// Save off the module.
	//
	GetRubyProvGlobals()->DbgScriptModule = module;
}

