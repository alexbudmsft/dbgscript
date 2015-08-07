//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: typedobject.cpp
// @Author: alexbud
//
// Purpose:
//
//  TypedObject class for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#include "common.h"
#include "typedobject.h"

//------------------------------------------------------------------------------
// Function: TypedObject_free
//
// Description:
//
//  Frees a DbgScriptTypedObject object allocated by 'TypedObject_alloc'.
//  
// Returns:
//
// Notes:
//
static void
TypedObject_free(
	_In_ void* obj)
{
	DbgScriptTypedObject* o = (DbgScriptTypedObject*)obj;
	delete o;
}

//------------------------------------------------------------------------------
// Function: TypedObject_alloc
//
// Description:
//
//  Allocates a Ruby-wrapped DbgScriptTypedObject object.
//  
// Returns:
//
// Notes:
//
static VALUE
TypedObject_alloc(
	_In_ VALUE klass)
{
	DbgScriptTypedObject* obj = new DbgScriptTypedObject;
	memset(obj, 0, sizeof(*obj));

	return Data_Wrap_Struct(klass, nullptr /* mark */, TypedObject_free, obj);
}

void
Init_TypedObject()
{
	VALUE typedObjectClass = rb_define_class_under(
		GetRubyProvGlobals()->DbgScriptModule,
		"TypedObject",
		rb_cObject);
	
	rb_define_alloc_func(typedObjectClass, TypedObject_alloc);

	// Prevent scripter from instantiating directly.
	//
	LockDownClass(typedObjectClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->TypedObjectClass = typedObjectClass;
}
