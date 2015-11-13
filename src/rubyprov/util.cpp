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
_Check_return_ VALUE
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
		delete [] buf;  // Don't leak.
		rb_raise(rb_eRuntimeError, "UtilReadBytes failed. Error 0x%08x.", hr);
	}
	
	VALUE ret = rb_str_new(buf, cbActual);

	// Ruby makes an internal copy of the input buffer.
	//
	delete [] buf;
	return ret;
}

//------------------------------------------------------------------------------
// Function: RbReadString
//
// Description:
//
//  Read 'count' bytes from 'addr'
//  
// Returns:
//
// Notes:
//
_Check_return_ VALUE
RbReadString(
	_In_ UINT64 addr,
	_In_ int count)
{
	HRESULT hr = S_OK;

	char buf[MAX_READ_STRING_LEN] = {};
	if (!count || count > MAX_READ_STRING_LEN - 1)
	{
		rb_raise(rb_eArgError, "count supports at most %d and can't be 0", MAX_READ_STRING_LEN - 1);
	}
	
	hr = UtilReadAnsiString(GetRubyProvGlobals()->HostCtxt, addr, STRING_AND_CCH(buf), count);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilReadAnsiString failed. Error 0x%08x.", hr);
	}

	return rb_str_new_cstr(buf);
}

//------------------------------------------------------------------------------
// Function: RbReadWideString
//
// Description:
//
//  Read 'count' bytes from 'addr'
//  
// Returns:
//
// Notes:
//
_Check_return_ VALUE
RbReadWideString(
	_In_ UINT64 addr,
	_In_ int count)
{
	WCHAR buf[MAX_READ_STRING_LEN] = {};
	char utf8buf[MAX_READ_STRING_LEN * sizeof(WCHAR)] = {};
	
	if (!count || count > MAX_READ_STRING_LEN - 1)
	{
		rb_raise(rb_eArgError, "count supports at most %d and can't be 0", MAX_READ_STRING_LEN - 1);
	}
	
	HRESULT hr = UtilReadWideString(GetRubyProvGlobals()->HostCtxt, addr, STRING_AND_CCH(buf), count);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilReadWideString failed. Error 0x%08x.", hr);
	}

	// Convert to UTF8.
	//
	const int cbWritten = WideCharToMultiByte(
		CP_UTF8, 0, buf, -1, utf8buf, sizeof utf8buf, nullptr, nullptr);
	if (!cbWritten)
	{
		DWORD err = GetLastError();
		rb_raise(rb_eRuntimeError, "WideCharToMultiByte failed. Error %d.", err);
	}

	return rb_utf8_str_new_cstr(utf8buf);
}
