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

//------------------------------------------------------------------------------
// Function: dbgscript_teb
//
// Description:
//
//  Get the current thread's TEB in the target process.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Returns:
//
//  One result: lua_Integer representing the TEB.
//
// Notes:
//
static int
dbgscript_teb(lua_State* L)
{
	lua_pushinteger(L, 1234);

	return 1;
}

// Functions in module.
//
static const luaL_Reg dbgscript[] =
{
	{"teb", dbgscript_teb},
	
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
