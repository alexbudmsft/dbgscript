//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: common.h
// @Author: alexbud
//
// Purpose:
//
//  Common header for Lua provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#pragma once

#include <hostcontext.h>
#include "../common.h"
#include "../support/util.h"

// Enable Lua StdIO redirection extension.
//
#define LUA_REDIRECT

#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>

struct LuaProvGlobals
{
	HMODULE HModule;
	DbgScriptHostContext* HostCtxt;

	// Input buffer for getc support.
	//
	char InputBuf[2048];

	// Next input char to be returned from getc callback.
	//
	int NextInputChar;

	ULONG ValidInputChars;
};

_Check_return_ LuaProvGlobals*
GetLuaProvGlobals();
