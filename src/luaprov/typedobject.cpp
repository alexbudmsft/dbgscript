//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: typedobject.cpp
// @Author: alexbud
//
// Purpose:
//
//  TypedObject class for Lua.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "typedobject.h"
#include "classprop.h"
#include <strsafe.h>

#define TYPED_OBJECT_METATABLE  "dbgscript.TypedObject"

//------------------------------------------------------------------------------
// Function: TypedObject_new
//
// Description:
//
//  Allocate a typed object.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Returns:
//
//  One result: User datum representing the typed object.
//
// Notes:
//
static int
TypedObject_new(lua_State* L)
{
	// Allocate a user datum.
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		lua_newuserdata(L, sizeof(DbgScriptTypedObject));

	// Bind userdatum to our metatable.
	//
	luaL_getmetatable(L, TYPED_OBJECT_METATABLE);
	lua_setmetatable(L, -2);
	
	// Initialize fields, etc.
	//
	StringCchCopyA(STRING_AND_CCH(typObj->Name), "abc");

	// One element on the stack: the user datum.
	//
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_index
//
// Description:
//
//  Indexer for typed objects.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the object.
//  Param 2 is the key. Key could be an int or string.
//
// Returns:
//
//  One result: Varies.
//
// Notes:
//
static int
TypedObject_index(lua_State* L)
{
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	if (lua_isinteger(L, 2))
	{
		// Attempt array access.
		//
		return 0; // TODO
	}
	else
	{
		// Attempt dbgeng-field or Lua-class property access.
		//
		luaL_checktype(L, 2, LUA_TSTRING);
		
		//
		// First check if the key is a Lua-class field.
		//
		
		// Push the C function we're about to call.
		//
		lua_pushcfunction(L, LuaClassPropIndexer);

		// Param 1: Get the typed object's metatable and push it on the top.
		//
		lua_getmetatable(L, 1);

		// Param 2: Copy the key and push it on top.
		//
		lua_pushvalue(L, 2);

		// Call it.
		//
		lua_call(L, 2, 1);

		if (!lua_isnil(L, -1))
		{
			return 1;
		}
		else
		{
			// Else fall back to dbg-eng field lookup.
			//
			return 0; // TODO
		}
	}
}

// Static (class) methods.
//
static const luaL_Reg g_typedObjectFunc[] =
{
	{"new", TypedObject_new},  // TEMP!
	
	{nullptr, nullptr}  // sentinel.
};

static const LuaClassProperty x_TypedObjectProps[] =
{
	// Name   Getter   Setter
	// -------------------------
	{ "name", nullptr, nullptr },
	{ "size", nullptr, nullptr },
	{ "type", nullptr, nullptr },
};

// Instance methods.
//
static const luaL_Reg g_typedObjectMethods[] =
{
	{"__index", TypedObject_index},  // indexer. Serves array, field and property access.
	
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
luaopen_TypedObject(lua_State* L)
{
	luaL_newmetatable(L, TYPED_OBJECT_METATABLE);

	// Set methods.
	//
	luaL_setfuncs(L, g_typedObjectMethods, 0);

	// Set properties.
	//
	LuaSetProperties(L, x_TypedObjectProps, _countof(x_TypedObjectProps));
	
	luaL_newlib(L, g_typedObjectFunc);
	return 1;  // Number of results.
}

