//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.cpp
// @Author: alexbud
//
// Purpose:
//
//  Utilities for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#include "common.h"

//------------------------------------------------------------------------------
// Function: LockDownClass
//
// Description:
//
//  Lock down key methods in a class.
//  
// Returns:
//
// Notes:
//
void
LockDownClass(
	_In_ VALUE klass)
{
	// Prevent scripter from instantiating directly.
	//
	// 'rb_undef_method' undefs an *instance* method of the given class,
	// so for 'new' which is a 'class method', we have to get the
	// object's class.
	//
	rb_undef_method(CLASS_OF(klass), "new");
	
	rb_undef_method(klass, "dup");
	rb_undef_method(klass, "clone");
	
}

//------------------------------------------------------------------------------
// Function: RbReadBytes
//
// Description:
//
//  Read 'count' bytes from 'addr'
//  
// Returns:
//
// Notes:
//
VALUE
RbReadBytes(
	_In_ UINT64 addr,
	_In_ VALUE count)
{
	const ULONG cb = NUM2ULONG(count);
	char* buf = new char[cb];
	if (!buf)
	{
		rb_raise(rb_eNoMemError, "Couldn't allocate buffer.");
	}
	
	ULONG cbActual = 0;
	HRESULT hr = UtilReadBytes(
		GetRubyProvGlobals()->HostCtxt, addr, buf, cb, &cbActual);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilReadBytes failed. Error 0x%08x.", hr);
	}
	VALUE ret = rb_str_new(buf, cbActual);

	// Ruby makes an internal copy of the input buffer.
	//
	delete [] buf;
	
	return ret;
}

