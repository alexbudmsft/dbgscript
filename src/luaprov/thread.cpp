//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: thread.cpp
// @Author: alexbud
//
// Purpose:
//
//  Thread class for Lua.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "thread.h"
#include "classprop.h"
#include "util.h"

#define THREAD_METATABLE  "dbgscript.Thread"

//------------------------------------------------------------------------------
// Function: AllocThreadObject
//
// Description:
//
//  Helper to allocate a thread object.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Returns:
//
//  One result: User datum representing the thread object.
//
// Notes:
//
DbgScriptThread*
AllocThreadObject(
	_In_ lua_State* L)
{
	// Allocate a user datum.
	//
	DbgScriptThread* thd = (DbgScriptThread*)
		lua_newuserdata(L, sizeof(DbgScriptThread));

	// Bind userdatum to our metatable.
	//
	luaL_getmetatable(L, THREAD_METATABLE);
	lua_setmetatable(L, -2);

	return thd;
}

//------------------------------------------------------------------------------
// Function: Thread_getEngineId
//
// Description:
//
//  Get the engine thread id.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the thread object.
//
// Returns:
//
//  One result: Engine id.
//
// Notes:
//
static int
Thread_getEngineId(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptThread* thd = (DbgScriptThread*)
		luaL_checkudata(L, 1, THREAD_METATABLE);

	lua_pushinteger(L, thd->EngineId);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: Thread_getThreadId
//
// Description:
//
//  Get the system thread id.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the thread object.
//
// Returns:
//
//  One result: Thread id.
//
// Notes:
//
static int
Thread_getThreadId(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptThread* thd = (DbgScriptThread*)
		luaL_checkudata(L, 1, THREAD_METATABLE);

	lua_pushinteger(L, thd->ThreadId);
	
	return 1;
}


// Static (class) methods.
//
static const luaL_Reg g_threadFunctions[] =
{
	// None.
	//
	
	{nullptr, nullptr}  // sentinel.
};

// Class Properties.
//
static const LuaClassProperty x_ThreadProps[] =
{
	// Name   Getter               Setter
	// -----------------------------------------------------------
	{ "engineId", Thread_getEngineId, nullptr },
	{ "threadId", Thread_getThreadId, nullptr },
};

// Instance methods.
//
static const luaL_Reg g_threadMethods[] =
{
	{"__index", LuaClassPropIndexer},  // indexer. Handles properties/methods.
	
	{nullptr, nullptr}  // sentinel.
};

//------------------------------------------------------------------------------
// Function: luaopen_Thread
//
// Description:
//
//  'Open' routine for Thread Lua class.
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
luaopen_Thread(lua_State* L)
{
	luaL_newmetatable(L, THREAD_METATABLE);

	// Set methods.
	//
	luaL_setfuncs(L, g_threadMethods, 0);

	// Set properties.
	//
	LuaSetProperties(L, x_ThreadProps, _countof(x_ThreadProps));
	
	luaL_newlib(L, g_threadFunctions);
	return 1;  // Number of results.
}

