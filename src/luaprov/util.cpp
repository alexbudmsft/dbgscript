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

//------------------------------------------------------------------------------
// Function: LuaReadWideString
//
// Description:
//
//  Read a wide string from the target process as a Lua string up to 'count'
//  chars.
//  
// Returns:
//
//  Pushes new string on stack and returns 1 result.
//
// Notes:
//
int
LuaReadWideString(
	_In_ lua_State* L,
	_In_ UINT64 addr,
	_In_ int count)
{
	WCHAR buf[MAX_READ_STRING_LEN] = {};
	char utf8buf[MAX_READ_STRING_LEN * sizeof(WCHAR)] = {};
	if (!count || count > MAX_READ_STRING_LEN - 1)
	{
		return luaL_error(L, "count supports at most %d and can't be 0", MAX_READ_STRING_LEN - 1);
	}
	
	HRESULT hr = UtilReadWideString(
		GetLuaProvGlobals()->HostCtxt, addr, STRING_AND_CCH(buf), count);
	if (FAILED(hr))
	{
		return LuaError(L, "UtilReadWideString failed. Error 0x%08x.", hr);
	}
	
	// Convert to UTF8.
	//
	const int cbWritten = WideCharToMultiByte(
		CP_UTF8, 0, buf, -1, utf8buf, sizeof utf8buf, nullptr, nullptr);
	if (!cbWritten)
	{
		DWORD err = GetLastError();
		luaL_error(L, "WideCharToMultiByte failed. Error %d.", err);
	}
	
	lua_pushstring(L, utf8buf);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: LuaReadString
//
// Description:
//
//  Read a wide string from the target process as a Lua string up to 'count'
//  chars.
//  
// Returns:
//
//  Pushes new string on stack and returns 1 result.
//
// Notes:
//
int
LuaReadString(
	_In_ lua_State* L,
	_In_ UINT64 addr,
	_In_ int count)
{
	char buf[MAX_READ_STRING_LEN] = {};
	if (!count || count > MAX_READ_STRING_LEN - 1)
	{
		return luaL_error(L, "count supports at most %d and can't be 0", MAX_READ_STRING_LEN - 1);
	}
	
	HRESULT hr = UtilReadAnsiString(
		GetLuaProvGlobals()->HostCtxt, addr, STRING_AND_CCH(buf), count);
	if (FAILED(hr))
	{
		return LuaError(L, "UtilReadAnsiString failed. Error 0x%08x.", hr);
	}
	
	lua_pushstring(L, buf);
	
	return 1;
}

