//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: stackframe.cpp
// @Author: alexbud
//
// Purpose:
//
//  StackFrame class for Lua.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "stackframe.h"
#include "classprop.h"
#include "util.h"

#define STACKFRAME_METATABLE  "dbgscript.StackFrame"

//------------------------------------------------------------------------------
// Function: AllocStackFrameObject
//
// Description:
//
//  Helper to allocate a stackframe object.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Returns:
//
//  One result: User datum representing the stackframe object.
//
// Notes:
//
DbgScriptStackFrame*
AllocStackFrameObject(
	_In_ lua_State* L)
{
	// Allocate a user datum.
	//
	DbgScriptStackFrame* thd = (DbgScriptStackFrame*)
		lua_newuserdata(L, sizeof(DbgScriptStackFrame));

	// Bind userdatum to our metatable.
	//
	luaL_getmetatable(L, STACKFRAME_METATABLE);
	lua_setmetatable(L, -2);

	return thd;
}

//------------------------------------------------------------------------------
// Function: StackFrame_frameNumber
//
// Description:
//
//  Get the stackframe number.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the stackframe object.
//
// Returns:
//
//  One result: frame num.
//
// Notes:
//
static int
StackFrame_frameNumber(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptStackFrame* frame = (DbgScriptStackFrame*)
		luaL_checkudata(L, 1, STACKFRAME_METATABLE);

	lua_pushinteger(L, frame->FrameNumber);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: StackFrame_instructionOffset
//
// Description:
//
//  Get the instruction offset.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the stackframe object.
//
// Returns:
//
//  One result: instruction offset.
//
// Notes:
//
static int
StackFrame_instructionOffset(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptStackFrame* frame = (DbgScriptStackFrame*)
		luaL_checkudata(L, 1, STACKFRAME_METATABLE);

	lua_pushinteger(L, frame->InstructionOffset);
	
	return 1;
}

// Static (class) methods.
//
static const luaL_Reg g_stackFrameFunctions[] =
{
	// None.
	//
	
	{nullptr, nullptr}  // sentinel.
};

// Class Properties.
//
static const LuaClassProperty x_StackFrameProps[] =
{
	// Name          Getter                         Setter
	// -----------------------------------------------------------
	{ "frameNumber", StackFrame_frameNumber, nullptr },
	{ "instructionOffset", StackFrame_instructionOffset, nullptr },
};

// Instance methods.
//
static const luaL_Reg g_stackFrameMethods[] =
{
	{"__index", LuaClassPropIndexer},  // indexer. Handles properties/methods.
	
	{nullptr, nullptr}  // sentinel.
};

//------------------------------------------------------------------------------
// Function: luaopen_StackFrame
//
// Description:
//
//  'Open' routine for StackFrame Lua class.
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
luaopen_StackFrame(lua_State* L)
{
	luaL_newmetatable(L, STACKFRAME_METATABLE);

	// Set methods.
	//
	luaL_setfuncs(L, g_stackFrameMethods, 0);

	// Set properties.
	//
	LuaSetProperties(L, x_StackFrameProps, _countof(x_StackFrameProps));
	
	luaL_newlib(L, g_stackFrameFunctions);
	return 1;  // Number of results.
}

