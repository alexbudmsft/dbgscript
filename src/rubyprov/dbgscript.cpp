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

	// Allocate a Typed Object.
	//
	VALUE typObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->TypedObjectClass);

	DbgScriptTypedObject* obj = nullptr;

	Data_Get_Struct(typObj, DbgScriptTypedObject, obj);

	HRESULT hr = DsInitializeTypedObject(
		hostCtxt,
		0 /* size */,
		nullptr /* name */,
		szType,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		ui64Addr,
		obj);
	if (FAILED(hr))
	{
		rb_raise(rb_eSystemCallError, "DsInitializeTypedObject failed. Error 0x%08x.", hr);
	}

	return typObj;
}

void
Init_DbgScript()
{
	VALUE module = rb_define_module("DbgScript");

	rb_define_module_function(
		module, "read_ptr", RUBY_METHOD_FUNC(DbgScript_read_ptr), 1);

	rb_define_module_function(
		module, "current_thread", RUBY_METHOD_FUNC(DbgScript_current_thread), 0);
	
	rb_define_module_function(
		module, "create_typed_object", RUBY_METHOD_FUNC(DbgScript_create_typed_object), 2);
	
	// Save off the module.
	//
	GetRubyProvGlobals()->DbgScriptModule = module;
}

