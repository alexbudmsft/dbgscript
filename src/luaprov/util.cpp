//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.cpp
// @Author: alexbud
//
// Purpose:
//
//  Lua Provider utilities.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include <strsafe.h>
#include <stdarg.h>
#include "util.h"

//------------------------------------------------------------------------------
// Function: LuaError
//
// Description:
//
//  Raise a formatted error message.
//  
// Returns:
//
// Notes:
//
//  This function allows more elaborate format specifiers than luaL_error.
//
int
LuaError(
	_In_ lua_State* L,
	_In_z_ const char* fmt,
	...)
{
	char buf[1024];
	va_list arg;
	va_start(arg, fmt);
	StringCchVPrintfA(STRING_AND_CCH(buf), fmt, arg);
	va_end(arg);

	return luaL_error(L, "%s", buf);
}

//------------------------------------------------------------------------------
// Function: LuaReadBytes
//
// Description:
//
//  Read 'count' bytes from 'addr'
//  
// Returns:
//
//  Pushes result on stack and returns number of results.
//
// Notes:
//
//  Lua strings, like Ruby, can carry arbitrary payloads.
//
int
LuaReadBytes(
	_In_ lua_State* L,
	_In_ UINT64 addr,
	_In_ ULONG count)
{
	char* buf = new char[count];
	if (!buf)
	{
		return luaL_error(L, "Couldn't allocate buffer.");
	}
	
	ULONG cbActual = 0;
	HRESULT hr = UtilReadBytes(
		GetLuaProvGlobals()->HostCtxt, addr, buf, count, &cbActual);
	if (FAILED(hr))
	{
		delete [] buf;  // Don't leak.
		return LuaError(L, "UtilReadBytes failed. Error 0x%08x.", hr);
	}

	// Push result on the stack.
	//
	lua_pushlstring(L, buf, cbActual);

	// Lua makes an internal copy of the input buffer.
	//
	delete [] buf;
	
	return 1;
}

