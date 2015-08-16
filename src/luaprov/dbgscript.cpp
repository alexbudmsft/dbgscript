//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dbgscript.cpp
// @Author: alexbud
//
// Purpose:
//
//  DbgScript module for Lua Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#include "dbgscript.h"
#include "util.h"
#include "typedobject.h"
#include "../support/symcache.h"

//------------------------------------------------------------------------------
// Function: dbgscript_createTypedObject
//
// Description:
//
//  Created a Typed Object from an address and type.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Type (string)
//  2 - Address (integer)
//
// Returns:
//
//  One result: Typed Object.
//
// Notes:
//
static int
dbgscript_createTypedObject(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	// Explictly check for string, not convertible to string.
	//
	luaL_checktype(L, 1, LUA_TSTRING);
	luaL_checkinteger(L, 2);

	const char *typeName = lua_tostring(L, 1);
	UINT64 addr = lua_tointeger(L, 2);
	
	// Lookup typeid/moduleBase from type name.
	//
	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, typeName);
	if (!typeInfo)
	{
		return luaL_error(L, "Failed to get type id for type '%s'.", typeName);
	}

	AllocNewTypedObject(
		L, 0, nullptr, typeName, typeInfo->TypeId, typeInfo->ModuleBase, addr);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_createTypedObject
//
// Description:
//
//  Execute a debugger command.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Command (string)
//
// Returns:
//
//  No results.
//
// Notes:
//
static int
dbgscript_execCommand(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	HRESULT hr = UtilExecuteCommand(hostCtxt, lua_tostring(L, 1));
	if (FAILED(hr))
	{
		return LuaError(L, "UtilExecuteCommand failed. Error 0x%08x.", hr);
	}
	
	return 0;
}

//------------------------------------------------------------------------------
// Function: dbgscript_startBuffering
//
// Description:
//
//  Start buffering output.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  None.
//
// Returns:
//
//  No results.
//
// Notes:
//
static int
dbgscript_startBuffering(lua_State* /*L*/)
{
	// Currently this is single-threaded access, but will make it simpler in
	// case we ever have more than one concurrent client.
	//
	InterlockedIncrement(&GetLuaProvGlobals()->HostCtxt->IsBuffering);
	
	// No results.
	//
	return 0;
}

//------------------------------------------------------------------------------
// Function: dbgscript_stopBuffering
//
// Description:
//
//  Stop buffering output.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  None.
//
// Returns:
//
//  No results.
//
// Notes:
//
static int
dbgscript_stopBuffering(lua_State* L)
{
	// Currently this is single-threaded access, but will make it simpler in
	// case we ever have more than one concurrent client.
	//
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	const LONG newVal = InterlockedDecrement(&hostCtxt->IsBuffering);
	if (newVal < 0)
	{
		hostCtxt->IsBuffering = 0;
		return luaL_error(L, "can't stop buffering if it isn't started.");
	}

	// If the buffer refcount hit zero, flush remaining buffered content, if any.
	//
	if (newVal == 0)
	{
		UtilFlushMessageBuffer(hostCtxt);
	}
	
	// No results.
	//
	return 0;
}

// Functions in module.
//
static const luaL_Reg dbgscript[] =
{
	{"createTypedObject", dbgscript_createTypedObject},
	{"execCommand", dbgscript_execCommand},
	{"startBuffering", dbgscript_startBuffering},
	{"stopBuffering", dbgscript_stopBuffering},
	{nullptr, nullptr}  // sentinel.
};

//------------------------------------------------------------------------------
// Function: luaopen_dbgscript
//
// Description:
//
//  'Open' routine for dbgscript Lua module. This routine opens the module and
//  initializes it.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Returns:
//
//  int - number of results returned, per Lua convention.
//
// Notes:
//
int
luaopen_dbgscript(lua_State* L)
{
	luaL_newlib(L, dbgscript);
	return 1;  // Number of results.
}
