#include "typedobject.h"
#include <strsafe.h>

#define TYPED_OBJECT_METATABLE  "dbgscript.TypedObject"

#define LUA_PROPERTIES_KEY "__properties"

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
// Returns:
//
//  One result: Varies.
//
// Notes:
//
static int
TypedObject_index(lua_State* /*L*/)
{
	return 0;
}

// Static (class) methods.
//
static const luaL_Reg g_typedObjectFunc[] =
{
	{"new", TypedObject_new},  // TEMP!
	
	{nullptr, nullptr}  // sentinel.
};

// LuaClassProperty - simulate Python/C# properties with getters/setters.
//
struct LuaClassProperty
{
	// Name of property.
	//
	const char* Name;

	// Getter function. Takes an object, key.
	//
	lua_CFunction Get;

	// Setter function. Takes a object, key, value.
	//
	// May be NULL, which indicates a read-only property.
	//
	lua_CFunction Set;
};

static const LuaClassProperty x_TypedObjectProps[] =
{
	// Name   Getter   Setter
	// -------------------------
	{ "name", nullptr, nullptr },
	{ "size", nullptr, nullptr },
	{ "type", nullptr, nullptr },
};

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

		// Insert getter. (maybe NULL)
		//
		lua_pushcfunction(L, props[i].Get);
		
		lua_setfield(L, -2, "get");
		
		// Insert setter. (maybe NULL)
		//
		lua_pushcfunction(L, props[i].Set);
		lua_setfield(L, -2, "set");

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

