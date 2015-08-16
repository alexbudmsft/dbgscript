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
#include "util.h"

#define TYPED_OBJECT_METATABLE  "dbgscript.TypedObject"

//------------------------------------------------------------------------------
// Function: AllocTypedObject
//
// Description:
//
//  Helper to allocate a typed object.
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
static DbgScriptTypedObject*
allocTypedObject(
	_In_ lua_State* L)
{
	// Allocate a user datum.
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		lua_newuserdata(L, sizeof(DbgScriptTypedObject));

	// Bind userdatum to our metatable.
	//
	luaL_getmetatable(L, TYPED_OBJECT_METATABLE);
	lua_setmetatable(L, -2);

	return typObj;
}

//------------------------------------------------------------------------------
// Function: AllocTypedObject
//
// Description:
//
//  Helper to allocate a typed object.
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
void
allocSubTypedObject(
	_In_ lua_State* L,
	_In_z_ const char* name,
	_In_ const DEBUG_TYPED_DATA* typedData)
{
	DbgScriptTypedObject* typObj = allocTypedObject(L);
	
	HRESULT hr = DsWrapTypedData(
		GetLuaProvGlobals()->HostCtxt, name, typedData, typObj);
	if (FAILED(hr))
	{
		LuaError(
			L, "DsWrapTypedData failed. Error 0x%08x.", hr);
	}
}

//------------------------------------------------------------------------------
// Function: AllocTypedObject
//
// Description:
//
//  Helper to allocate a typed object.
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
void
AllocNewTypedObject(
	_In_ lua_State* L,
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_z_ const char* type,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress)
{
	DbgScriptTypedObject* typObj = allocTypedObject(L);
	
	// Initialize fields.
	//
	HRESULT hr = DsInitializeTypedObject(
		GetLuaProvGlobals()->HostCtxt,
		size,
		name,
		type,
		typeId,
		moduleBase,
		virtualAddress,
		typObj);
	if (FAILED(hr))
	{
		LuaError(
			L, "DsInitializeTypedObject failed. Error 0x%08x.", hr);
	}
}

//------------------------------------------------------------------------------
// Function: TypedObject_index
//
// Description:
//
//  Read indexer for typed objects.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
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
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	if (lua_isinteger(L, 2))
	{
		// Attempt array access.
		//
		return 0; // TODO
	}
	else
	{
		// Attempt dbgeng-field or Lua-class property access.  Explictly
		// check for string, not convertible to string.
		//
		luaL_checktype(L, 2, LUA_TSTRING);
		
		//
		// First check if the key is a Lua-class field.
		//
		
		// Push the C function we're about to call.
		//
		lua_pushcfunction(L, LuaClassPropIndexer);

		// Param 1: push the typed object on the top.
		//
		lua_pushvalue(L, 1);

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
			const char* fieldName = lua_tostring(L, 2);
			
			DEBUG_TYPED_DATA typedData = {0};
			HRESULT hr = DsTypedObjectGetField(
				GetLuaProvGlobals()->HostCtxt,
				typObj,
				fieldName,
				&typedData);
			if (FAILED(hr))
			{
				return LuaError(
					L, "DsTypedObjectGetField failed. Error 0x%08x.", hr);
			}

			// This will push the new user datum on the stack.
			//
			allocSubTypedObject(L, fieldName, &typedData);
			
			return 1;
		}
	}
}

//------------------------------------------------------------------------------
// Function: TypedObject_getname
//
// Description:
//
//  Get
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the typed object.
//
// Returns:
//
//  One result: The name of the TypedObject.
//
// Notes:
//
static int
TypedObject_getname(lua_State* L)
{
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	lua_pushstring(L, typObj->Name);
	
	return 1;
}

// Static (class) methods.
//
static const luaL_Reg g_typedObjectFunc[] =
{
	// None.
	//
	
	{nullptr, nullptr}  // sentinel.
};

static const LuaClassProperty x_TypedObjectProps[] =
{
	// Name   Getter   Setter
	// -------------------------
	{ "name", TypedObject_getname, nullptr },
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

