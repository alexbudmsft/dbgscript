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
#include "thread.h"
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
	UINT64 addr = luaL_checkinteger(L, 2);
	
	// Lookup typeid/moduleBase from type name.
	//
	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, typeName);
	if (!typeInfo)
	{
		return luaL_error(L, "Failed to get type id for type '%s'.", typeName);
	}

	AllocNewTypedObject(
		L, 0, nullptr, typeInfo->TypeId, typeInfo->ModuleBase, addr);
	
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

//------------------------------------------------------------------------------
// Function: dbgscript_currentThread
//
// Description:
//
//  Get current thread.
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
dbgscript_currentThread(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Get TEB from debug client.
	//
	ULONG engineThreadId = 0;
	ULONG systemThreadId = 0;
	HRESULT hr = hostCtxt->DebugSysObj->GetCurrentThreadId(&engineThreadId);
	if (FAILED(hr))
	{
		return LuaError(
			L, "Failed to get engine thread id. Error 0x%08x.", hr);
	}

	hr = hostCtxt->DebugSysObj->GetCurrentThreadSystemId(&systemThreadId);
	if (FAILED(hr))
	{
		return LuaError(
			L, "Failed to get system thread id. Error 0x%08x.", hr);
	}
	
	DbgScriptThread* thd = AllocThreadObject(L);
	thd->EngineId = engineThreadId;
	thd->ThreadId = systemThreadId;
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_getThreads
//
// Description:
//
//  Get current thread.
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
//  An array of thread objects (table)
//
// Notes:
//
static int
dbgscript_getThreads(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	ULONG cThreads = 0;
	ULONG* engineThreadIds = nullptr;
	ULONG* sysThreadIds = nullptr;
	HRESULT hr = UtilCountThreads(hostCtxt, &cThreads);
	if (FAILED(hr))
	{
		return LuaError(L, "UtilCountThreads failed. Error 0x%08x.", hr);
	}

	// Get list of thread IDs.
	//
	engineThreadIds = new ULONG[cThreads];
	sysThreadIds = new ULONG[cThreads];

	hr = UtilEnumThreads(hostCtxt, cThreads, engineThreadIds, sysThreadIds);
	if (FAILED(hr))
	{
		return LuaError(L, "UtilEnumThreads failed. Error 0x%08x.", hr);
	}

	lua_createtable(L, cThreads /* array elems */, 0 /* hash elems */);

	// Build a tuple of Thread objects.
	//
	for (ULONG i = 0; i < cThreads; ++i)
	{
		DbgScriptThread* thd = AllocThreadObject(L);
		thd->EngineId = engineThreadIds[i];
		thd->ThreadId = sysThreadIds[i];

		// Insert into i'th index. In Lua, it is customary to start arrays
		// at index 1, not 0.
		//
		lua_rawseti(L, -2, i + 1);
	}

	delete[] engineThreadIds;
	delete[] sysThreadIds;

	// Return the table.
	//
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_getGlobal
//
// Description:
//
//  Get global object.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Name of object (string).
//
// Returns:
//
//  The global typed object.
//
// Notes:
//
static int
dbgscript_getGlobal(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	const char* symbol = lua_tostring(L, 1);
	UINT64 addr = 0;
	HRESULT hr = hostCtxt->DebugSymbols->GetOffsetByName(symbol, &addr);
	if (FAILED(hr))
	{
		return LuaError(L, "Failed to get virtual address for symbol '%s'. Error 0x%08x.", symbol, hr);
	}

	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, symbol);
	if (!typeInfo)
	{
		return LuaError(L, "Failed to get type id for type '%s'.", symbol);
	}

	AllocNewTypedObject(
		L,
		0,
		symbol,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		addr);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_resolveEnum
//
// Description:
//
//  Resolve an enum against a value.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Name of enum (string).
//  2 - Value (int).
//
// Returns:
//
//  The name of the enumerant (string).
//
// Notes:
//
static int
dbgscript_resolveEnum(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	const char* enumTypeName = lua_tostring(L, 1);
	const UINT64 value = luaL_checkinteger(L, 2);
	
	char enumElementName[MAX_SYMBOL_NAME_LEN] = {};

	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, enumTypeName);
	if (!typeInfo)
	{
		return LuaError(L, "Failed to get type id for type '%s'.", enumTypeName);
	}

	HRESULT hr = hostCtxt->DebugSymbols->GetConstantName(
		typeInfo->ModuleBase,
		typeInfo->TypeId,
		value,
		STRING_AND_CCH(enumElementName),
		nullptr);
	if (FAILED(hr))
	{
		return LuaError(L, "Failed to get element name for enum '%s' with value '%llu'. Error 0x%08x.", enumTypeName, value, hr);
	}

	lua_pushstring(L, enumElementName);
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_readPtr
//
// Description:
//
//  Resolve a pointer from the target's address space.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Address to read from (int)
//
// Returns:
//
//  The pointer value.
//
// Notes:
//
static int
dbgscript_readPtr(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const UINT64 addr = luaL_checkinteger(L, 1);
	UINT64 ptrVal = 0;

	HRESULT hr = UtilReadPointer(hostCtxt, addr, &ptrVal);
	if (FAILED(hr))
	{
		return LuaError(L, "Failed to read pointer value from address %p. Error 0x%08x.", addr, hr);
	}

	lua_pushinteger(L, ptrVal);
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_readBytes
//
// Synopsis:
// 
//  dbgscript.readBytes(addr, count) -> string
//
// Description:
//
//  Read 'count' bytes from 'addr'.
//
static int
dbgscript_readBytes(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const UINT64 addr = luaL_checkinteger(L, 1);
	const ULONG count = (ULONG)luaL_checkinteger(L, 2);
	return LuaReadBytes(L, addr, count);
}

//------------------------------------------------------------------------------
// Function: dbgscript_readString
//
// Synopsis:
// 
//  dbgscript.readString(addr [, count]) -> string
//
// Description:
//
//  Read a string from 'addr' (up to 'count' bytes).
//
static int
dbgscript_readString(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const UINT64 addr = luaL_checkinteger(L, 1);
	const int count = (int)luaL_optinteger(L, 2, -1 /* default val */);
	return LuaReadString(L, addr, count);
}

//------------------------------------------------------------------------------
// Function: dbgscript_readWideString
//
// Synopsis:
// 
//  dbgscript.readWideString(addr [, count]) -> string
//
// Description:
//
//  Read a wide string from 'addr' (up to 'count' bytes).
//
static int
dbgscript_readWideString(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const UINT64 addr = luaL_checkinteger(L, 1);
	const int count = (int)luaL_optinteger(L, 2, -1 /* default val */);
	return LuaReadWideString(L, addr, count);
}

//------------------------------------------------------------------------------
// Function: dbgscript_fieldOffset
//
// Synopsis:
// 
//  dbgscript.fieldOffset(type, field) -> integer
//
// Description:
//
//  Return the offset of 'field' in 'type'.
//
static int
dbgscript_fieldOffset(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const char* type = lua_tostring(L, 1);
	const char* field = lua_tostring(L, 2);
	
	ULONG offset = 0;
	HRESULT hr = UtilGetFieldOffset(hostCtxt, type, field, &offset);
	if (FAILED(hr))
	{
		return LuaError(
			L,
			"Failed to get field offset for type '%s' and field '%s'. Error 0x%08x.",
			type,
			field,
			hr);
	}

	lua_pushinteger(L, offset);
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_getNearestSym
//
// Description:
//
//  Lookup the nearest symbol from an address.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Address to probe (int)
//
// Returns:
//
//  The nearest symbol name.
//
// Notes:
//
static int
dbgscript_getNearestSym(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	const UINT64 addr = luaL_checkinteger(L, 1);
	char name[MAX_SYMBOL_NAME_LEN] = {};

	HRESULT hr = UtilGetNearestSymbol(hostCtxt, addr, name);
	if (FAILED(hr))
	{
		return LuaError(L, "Failed to resolve symbol from address %p. Error 0x%08x.", addr, hr);
	}

	lua_pushstring(L, name);
	return 1;
}

//------------------------------------------------------------------------------
// Function: dbgscript_getPeb
//
// Description:
//
//  Get the address of the current process' PEB.
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
//  The address of the PEB.
//
// Notes:
//
static int
dbgscript_getPeb(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);

	UINT64 addr = 0;
	HRESULT hr = UtilGetPeb(hostCtxt, &addr);
	if (FAILED(hr))
	{
		return LuaError(L, "Failed to get PEB. Error 0x%08x.", hr);
	}

	lua_pushinteger(L, addr);
	return 1;
}

// Functions in module.
//
static const luaL_Reg dbgscript[] =
{
	{"createTypedObject", dbgscript_createTypedObject},
	{"execCommand", dbgscript_execCommand},
	{"startBuffering", dbgscript_startBuffering},
	{"stopBuffering", dbgscript_stopBuffering},
	{"currentThread", dbgscript_currentThread},
	{"getThreads", dbgscript_getThreads},
	{"getGlobal", dbgscript_getGlobal},
	{"resolveEnum", dbgscript_resolveEnum},
	{"readPtr", dbgscript_readPtr},
	{"fieldOffset", dbgscript_fieldOffset},
	{"getNearestSym", dbgscript_getNearestSym},
	{"getPeb", dbgscript_getPeb},
	{"readBytes", dbgscript_readBytes},
	{"readString", dbgscript_readString},
	{"readWideString", dbgscript_readWideString},
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
