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
#include "typedobject.h"
#include "util.h"

#define STACKFRAME_METATABLE  "dbgscript.StackFrame"

// Indices for uservalue associated with StackFrame object.
//
enum StackFrameUserValues
{
	StackFrameUserValue_Thread,

	// Add before this line.
	//
	NumStackFrameUserValues
};

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
//  thdIdx - Positive index of the thread userdatum on entry to the function.
//
// Returns:
//
//  One result: User datum representing the stackframe object.
//
// Notes:
//
DbgScriptStackFrame*
AllocStackFrameObject(
	_In_ lua_State* L,
	_In_ int thdIdx)
{
	// Allocate a user datum.
	//
	DbgScriptStackFrame* frame = (DbgScriptStackFrame*)
		lua_newuserdata(L, sizeof(DbgScriptStackFrame));

	// Bind userdatum to our metatable.
	//
	luaL_getmetatable(L, STACKFRAME_METATABLE);
	lua_setmetatable(L, -2);

	// Create a user table to associate with the stack frame object to hold
	// references to things we don't want the GC to collect while we're alive.
	//
	lua_createtable(L, NumStackFrameUserValues, 0);

	// Associate thread userdatum this StackFrame object to prevent its premature
	// collection. Can't store lightuserdata since that doesn't prevent GC.
	//
	lua_pushvalue(L, thdIdx);

	// Customary to have one-based index.
	//
	lua_rawseti(L, -2, StackFrameUserValue_Thread + 1);

	lua_setuservalue(L, -2);

	return frame;
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

//------------------------------------------------------------------------------
// Function: buildArrayFromLocals
//
// Description:
//
//  Callback to UtilEnumStackFrameVariables which appends each local to
//  the given Lua array.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - the table in which to append locals.
//
// Returns:
//
//  One result: array of typed objects. (table)
//
// Notes:
//
static _Check_return_ HRESULT
buildArrayFromLocals(
	_In_ DEBUG_SYMBOL_ENTRY* entry,
	_In_z_ const char* symName,
	_In_z_ const char* typeName,
	_In_ ULONG idx,
	_In_opt_ void* ctxt)
{
	lua_State* L = (lua_State*)ctxt;

	AllocNewTypedObject(
		L,
		entry->Size,
		symName,
		typeName,
		entry->TypeId,
		entry->ModuleBase,
		entry->Offset);

	// In Lua, customary to have arrays start at 1.
	//
	lua_rawseti(L, -2, idx + 1);

	return S_OK;
}

//------------------------------------------------------------------------------
// Function: getVariablesHelper
//
// Description:
//
//  Helper to get args/locals.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - the stackframe object.
//
// Returns:
//
//  One result: array of typed objects. (table)
//
// Notes:
//
static int
getVariablesHelper(
	_In_ lua_State* L,
	_In_ ULONG flags)
{
	HRESULT hr = S_OK;
	ULONG numSym = 0;
	IDebugSymbolGroup2* symGrp = nullptr;
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptStackFrame* frame = (DbgScriptStackFrame*)
		luaL_checkudata(L, 1, STACKFRAME_METATABLE);

	lua_getuservalue(L, 1);

	// User value is on top of stack. Obtain thread pointer.
	//
	lua_rawgeti(L, -1, StackFrameUserValue_Thread + 1);
	const DbgScriptThread* thd = (DbgScriptThread*)lua_touserdata(L, -1);

	hr = UtilCountStackFrameVariables(
		hostCtxt,
		thd,
		frame,
		flags,
		&numSym,
		&symGrp);
	if (FAILED(hr))
	{
		return LuaError(L, "UtilCountStackFrameVariables failed. Error: 0x%08x", hr);
	}
	
	lua_createtable(L, numSym, 0);
	
	hr = UtilEnumStackFrameVariables(
		hostCtxt,
		symGrp,
		numSym,
		buildArrayFromLocals,
		L /* ctxt */);
	if (FAILED(hr))
	{
		return LuaError(L, "UtilEnumStackFrameVariables failed. Error: 0x%08x", hr);
	}

	return 1;
}

//------------------------------------------------------------------------------
// Function: StackFrame_getLocals
//
// Description:
//
//  Get locals and args in the frame.
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
//  One result: array of typed objects. (table)
//
// Notes:
//
static int
StackFrame_getLocals(
	_In_ lua_State* L)
{
	return getVariablesHelper(L, DEBUG_SCOPE_GROUP_LOCALS);
}

//------------------------------------------------------------------------------
// Function: StackFrame_getArgs
//
// Description:
//
//  Get args only in the frame.
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
//  One result: array of typed objects. (table)
//
// Notes:
//
static int
StackFrame_getArgs(
	_In_ lua_State* L)
{
	return getVariablesHelper(L, DEBUG_SCOPE_GROUP_ARGUMENTS);
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
	{"getLocals", StackFrame_getLocals},
	{"getArgs", StackFrame_getArgs},
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

