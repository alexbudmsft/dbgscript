#include "classprop.h"

// Key in which to store class properties.
//
#define LUA_PROPERTIES_KEY "__properties"

#define CLASSPROP_GET "get"
#define CLASSPROP_SET "set"

void
LuaBuildPropertyTable(
	_In_ lua_State* L,
	_In_reads_(count) const LuaClassProperty* props,
	_In_ int count)
{
	lua_createtable(L, 0 /* array elems */, count /* hash elems */);
	
	for (int i = 0; i < count; ++i)
	{
		// Allocate a new table for the getter/setter.
		//
		lua_createtable(L, 0 /* array elems */, 2 /* hash elems */);

		// Insert getter.
		//
		if (!props[i].Get)
		{
			lua_pushnil(L);
		}
		else
		{
			lua_pushcfunction(L, props[i].Get);
		}
		
		lua_setfield(L, -2, CLASSPROP_GET);
		
		// Insert setter.
		//
		if (!props[i].Set)
		{
			lua_pushnil(L);
		}
		else
		{
			lua_pushcfunction(L, props[i].Set);
		}
		lua_setfield(L, -2, CLASSPROP_SET);

		// The get/set table is on the top of the stack. Create a key whose
		// name is the property name with a value of our get/set table in the
		// input properties table.
		//
		lua_setfield(L, -2, props[i].Name);

		// lua_setfield always pops the key and value on return, so the top of
		// stack at each point is the properties table.
		//
	}
}

//------------------------------------------------------------------------------
// Function: LuaSetProperties
//
// Description:
//
//  Set class-style properties for a table.
//
// Parameters:
//
//  L - pointer to Lua state.
//  props - Property descriptors.
//  count - length of 'props' array.
//
// Input Stack:
//
//  1 - Table to set properties on.
//
// Returns:
//
//  void.
//
// Notes:
//
void
LuaSetProperties(
	_In_ lua_State* L,
	_In_reads_(count) const LuaClassProperty* props,
	_In_ int count)
{
	LuaBuildPropertyTable(L, props, count);

	// Now we have a properties table on top of the stack. Set the caller's
	// top-table properties field to point at this table.
	//
	lua_setfield(L, -2, LUA_PROPERTIES_KEY);
}

//------------------------------------------------------------------------------
// Function: LuaClassPropIndexer
//
// Description:
//
//  Indexer for class properties and methods.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  1 - Object to lookup properties on (table)
//  2 - Key (will be used to lookup property)
//
// Returns:
//
//  On success, returns the first result from getter (discards the rest).
//  On failure, returns nil.
//
//  TODO: Consider whether to return an error string as a second result on
//  failure.
//
// Notes:
//
int
LuaClassPropIndexer(lua_State* L)
{
	// First param must not be nil.
	//
	luaL_checkany(L, 1);
	
	// Key must be a string. Explictly check for string, not convertible to
	// string.
	//
	luaL_checktype(L, 2, LUA_TSTRING);

	// First param must have a metatable.
	//
	lua_getmetatable(L, 1);

	// Copy the key to the top of the stack.
	//
	lua_pushvalue(L, 2);

	// Check if an element exists with this key name. (Hopefully a method)
	//
	lua_gettable(L, -2);
	
	if (!lua_isnil(L, -1))
	{
		// Return element if present.
		//
		return 1;
	}

	// Pop the nil. Get the metatable back on the top.
	//
	lua_pop(L, 1);

	// Get the properties table from the metatable.
	//
	lua_getfield(L, -1, LUA_PROPERTIES_KEY);

	// properties key must be a table.
	//
	luaL_checktype(L, -1, LUA_TTABLE);

	// From the properties table, get the key sought.
	//
	// Copy the key to the top of the stack.
	//
	lua_pushvalue(L, 2);

	// Get the property.
	//
	lua_gettable(L, -2);
	
	if (lua_isnil(L, -1))
	{
		// No such property. Return nil.
		//
		return 1;
	}
	
	luaL_checktype(L, -1, LUA_TTABLE);
	
	// Property is another table, with a get/set method.
	//
	lua_getfield(L, -1, CLASSPROP_GET);

	if (lua_isnil(L, -1))
	{
		// No accessor.
		//
		return luaL_error(L, "no accessor '%s' on property '%s'.",
			CLASSPROP_GET, lua_tostring(L, 2));
	}
	
	luaL_checktype(L, -1, LUA_TFUNCTION);

	// Push the object as the first param.
	//
	lua_pushvalue(L, 1);

	lua_call(L, 1 /* num args */, 1 /* num results */);

	// Lua saves this many results, and clears the entire stack.
	//
	return 1;
}

