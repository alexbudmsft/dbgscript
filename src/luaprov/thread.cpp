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
#include "stackframe.h"

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

//------------------------------------------------------------------------------
// Function: Thread_getStack
//
// Description:
//
//  Get the stack.
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
//  One result: Array of frames (table)
//
// Notes:
//
static int
Thread_getStack(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptThread* thd = (DbgScriptThread*)
		luaL_checkudata(L, 1, THREAD_METATABLE);

	DEBUG_STACK_FRAME frames[512];
	ULONG framesFilled = 0;

	HRESULT hr = DsGetStackTrace(
		hostCtxt,
		thd,
		frames,
		_countof(frames),
		&framesFilled);
	if (FAILED(hr))
	{
		return LuaError(L, "DsGetStackTrace failed. Error 0x%08x.", hr);
	}

	lua_createtable(L, framesFilled, 0);

	// Build a tuple of StackFrame objects.
	//
	for (ULONG i = 0; i < framesFilled; ++i)
	{
		DbgScriptStackFrame* frame = AllocStackFrameObject(L, 1);
		frame->FrameNumber = frames[i].FrameNumber;
		frame->InstructionOffset = frames[i].InstructionOffset;

		// Lua arrays start at 1.
		//
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

//------------------------------------------------------------------------------
// Function: Thread_currentFrame
//
// Synopsis:
//
//  obj.currentFrame() -> StackFrame
//
// Description:
//
//  Return the current stack frame.
//
// TODO: Consider switching to the thread identified by 'self' before getting
// the current frame.
//
static int
Thread_currentFrame(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptThread* thd = (DbgScriptThread*)
		luaL_checkudata(L, 1, THREAD_METATABLE);

	thd;  // TODO: Use this to switch to this thread's context.
	
	// Allocate a StackFrame object.
	//
	DbgScriptStackFrame* dsframe = AllocStackFrameObject(L, 1);

	// Call the support library to fill in the frame information.
	//
	HRESULT hr = DsGetCurrentStackFrame(hostCtxt, dsframe);
	if (FAILED(hr))
	{
		return LuaError(L, "DsGetCurrentStackFrame failed. Error 0x%08x.", hr);
	}
	
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
	{"getStack", Thread_getStack},
	{"currentFrame", Thread_currentFrame},
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

