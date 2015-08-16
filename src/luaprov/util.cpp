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
