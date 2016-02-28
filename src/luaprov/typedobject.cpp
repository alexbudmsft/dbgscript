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
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = allocTypedObject(L);
	
	HRESULT hr = DsWrapTypedData(
		hostCtxt, name, typedData, typObj);
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
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_In_ bool wantPointer)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = allocTypedObject(L);
	
	// Initialize fields.
	//
	HRESULT hr = DsInitializeTypedObject(
		hostCtxt,
		size,
		name,
		typeId,
		moduleBase,
		virtualAddress,
		wantPointer,
		typObj);
	if (FAILED(hr))
	{
		LuaError(
			L, "DsInitializeTypedObject failed. Error 0x%08x.", hr);
	}
}

static void
checkTypedData(
	_In_ lua_State* L,
	_In_opt_ DbgScriptTypedObject* typObj)
{
	if (!typObj->TypedDataValid)
	{
		// This object has no typed data. It must have been a null ptr.
		//
		luaL_error(L, "object has no typed data. Can't get fields.");
	}
}

//------------------------------------------------------------------------------
// Function: getFieldHelper
//
// Description:
//
//  Helper to get a field from a typed object.
//
// Parameters:
//
//  L - pointer to Lua state.
//  typObj - Optional pointer to preinitialized DbgScriptTypedObject.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
//  Param 2 is the key. (string)
//
// Returns:
//
//  One result: the new typed object representing the field.
//
// Notes:
//
static int
getFieldHelper(
	_In_ lua_State* L,
	_In_opt_ DbgScriptTypedObject* typObj)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	if (!typObj)
	{
		// Validate that the first param was 'self'. I.e. a Userdatum of the right
		// type. (Having the right metatable).
		//
		typObj = (DbgScriptTypedObject*)
			luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);
		
		checkTypedData(L, typObj);
	}

	// Only want strings -- not anything convertible to a string (such as an int)
	//
	luaL_checktype(L, 2, LUA_TSTRING);
	
	const char* fieldName = lua_tostring(L, 2);
	
	DEBUG_TYPED_DATA typedData = {0};
	HRESULT hr = DsTypedObjectGetField(
		hostCtxt,
		typObj,
		fieldName,
		true,
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
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	HRESULT hr = S_OK;
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	if (lua_isinteger(L, 2))
	{
		checkTypedData(L, typObj);
		
		// Attempt array access.
		//
		DEBUG_TYPED_DATA typedData = {0};
		hr = DsTypedObjectGetArrayElement(
			hostCtxt,
			typObj,
			lua_tointeger(L, 2),
			&typedData);
		if (FAILED(hr))
		{
			return LuaError(
				L, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
		}
		
		// Array elements have no name.
		//
		allocSubTypedObject(L, ARRAY_ELEM_NAME, &typedData);
		return 1;
	}
	else
	{
		// Attempt dbgeng-field or Lua-class property access.  Explictly
		// check for string, not convertible to string.
		//
		luaL_checktype(L, 2, LUA_TSTRING);
		
		//
		// First check if the key is a Lua class property.
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

		// Call it. The return value could be the result of a getter call.
		// It could also be an existing field we fetched from the metatable
		// which lua will try to call (if the scripter attempted method call
		// syntax)
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
			checkTypedData(L, typObj);
			
			return getFieldHelper(L, typObj);
		}
	}
}

//------------------------------------------------------------------------------
// Function: TypedObject_len
//
// Description:
//
//  __len metamethod (#foo) to obtain length of array, if valid.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
//
// Returns:
//
//  One result: Length, if an array. Error otherwise.
//
// Notes:
//
static int
TypedObject_len(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	HRESULT hr = S_OK;
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);
	
	checkTypedData(L, typObj);

	if (typObj->TypedData.Tag != SymTagArrayType)
	{
		// Not array.
		//
		return luaL_error(L, "object not array.");
	}
	
	DEBUG_TYPED_DATA typedData = {0};
	hr = DsTypedObjectGetArrayElement(
		hostCtxt,
		typObj,
		0,  // index
		&typedData);
	if (FAILED(hr))
	{
		return LuaError(
			L, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
	}
	
	const ULONG elemSize = typedData.Size;
	
	const ULONG numElems = typObj->TypedData.Size / elemSize;
	
	lua_pushinteger(L, numElems);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: luaValueFromCValue
//
// Description:
//
//  Helper to push the appropriate Lua value onto the stack based on a C value
//  in the typed object.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
//
// Returns:
//
//  One result: value of primitive.
//
// Notes:
//
static int
luaValueFromCValue(
	_In_ lua_State* L,
	_In_ DbgScriptTypedObject* typObj)
{
	assert(typObj->ValueValid);
	assert(typObj->TypedDataValid);
	const TypedObjectValue* cValue = &typObj->Value;
	DEBUG_TYPED_DATA* typedData = &typObj->TypedData;

	if (typedData->Tag == SymTagPointerType)
	{
		lua_pushinteger(L, cValue->Value.UI64Val);
	}
	else if (typedData->Tag == SymTagEnum)
	{
		// Assume enum is type 'int'. I.e. 4 bytes. Common, but not necessarily
		// true all the time with advent of C++11.
		//
		lua_pushinteger(L, cValue->Value.DwVal);
	}
	else
	{
		assert(typedData->Tag == SymTagBaseType);

		switch (typedData->BaseTypeId)
		{
		case DNTYPE_CHAR:
			assert(typedData->Size == 1);
			lua_pushlstring(L, (const char*)&cValue->Value.ByteVal, 1);
			break;
		case DNTYPE_INT8:
		case DNTYPE_UINT8:
			assert(typedData->Size == 1);
			lua_pushinteger(L, cValue->Value.ByteVal);
			break;
		case DNTYPE_INT16:
		case DNTYPE_UINT16:
		case DNTYPE_WCHAR:
			static_assert(sizeof(WCHAR) == sizeof(WORD), "Assume WCHAR is 2 bytes");
			assert(typedData->Size == sizeof(WORD));
			lua_pushinteger(L, cValue->Value.WordVal);
			break;
		case DNTYPE_INT32:
		case DNTYPE_LONG32:
		case DNTYPE_UINT32:
		case DNTYPE_ULONG32:
			assert(typedData->Size == sizeof(DWORD));
			lua_pushinteger(L, cValue->Value.DwVal);
			break;
		case DNTYPE_INT64:
		case DNTYPE_UINT64:
			assert(typedData->Size == sizeof(UINT64));
			lua_pushinteger(L, cValue->Value.UI64Val);
			break;
		case DNTYPE_BOOL:  // C++ bool. Not Win32 BOOL.
			assert(typedData->Size == sizeof(bool));
			lua_pushboolean(L, cValue->Value.BoolVal);
			break;
		case DNTYPE_FLOAT32:
			assert(typedData->Size == sizeof(float));
			lua_pushnumber(L, cValue->Value.FloatVal);
			break;
		case DNTYPE_FLOAT64:
			assert(typedData->Size == sizeof(double));
			lua_pushnumber(L, cValue->Value.DoubleVal);
			break;
		default:
			return luaL_error(L, "Unsupported type id: %d (%s)",
				typedData->BaseTypeId,
				typObj->TypeName);
		}
	}

	// Number of results.
	//
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_getvalue
//
// Description:
//
//  Value getter for typed object, if object is a primitive type.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
//
// Returns:
//
//  One result: value of primitive type.
//
// Notes:
//
static int
TypedObject_getvalue(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	HRESULT hr = S_OK;
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);
	
	checkTypedData(L, typObj);

	if (!DsTypedObjectIsPrimitive(typObj))
	{
		return luaL_error(L, "not a primitive type.");
	}

	// Read the appropriate size from memory.
	//
	// What primitive type is bigger than 8 bytes?
	//
	ULONG cbRead = 0;
	assert(typObj->TypedData.Size <= 8);
	hr = hostCtxt->DebugSymbols->ReadTypedDataVirtual(
		typObj->TypedData.Offset,
		typObj->TypedData.ModBase,
		typObj->TypedData.TypeId,
		&typObj->Value.Value,
		sizeof(typObj->Value.Value),
		&cbRead);
	if (FAILED(hr))
	{
		return LuaError(L, "Failed to read typed data. Error 0x%08x.", hr);
	}
	assert(cbRead == typObj->TypedData.Size);

	// Value has been populated.
	//
	typObj->ValueValid = true;

	return luaValueFromCValue(L, typObj);
}

//------------------------------------------------------------------------------
// Function: TypedObject_getaddress
//
// Description:
//
//  Get address of typed object.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
//
// Returns:
//
//  One result: Virtual address of object.
//
// Notes:
//
static int
TypedObject_getaddress(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	lua_pushinteger(L, typObj->TypedData.Offset);
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_getname
//
// Description:
//
//  Get the name of the typed object.
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
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	lua_pushstring(L, typObj->Name);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_getsize
//
// Description:
//
//  Get the data size of the object.
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
//  One result: The size of the TypedObject.
//
// Notes:
//
static int
TypedObject_getsize(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	lua_pushinteger(L, typObj->TypedData.Size);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_gettype
//
// Description:
//
//  Get the type of the object.
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
//  One result: The type name of the TypedObject.
//
// Notes:
//
static int
TypedObject_gettype(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	lua_pushstring(L, typObj->TypeName);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_getmodule
//
// Description:
//
//  Get the module of the object.
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
//  One result: The module name of the TypedObject.
//
// Notes:
//
static int
TypedObject_getmodule(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	lua_pushstring(L, typObj->ModuleName);
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_getRuntimeObject
//
// Description:
//
//  Get the runtime type of the object.
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
//  One result: A new typed object representing the runtime type of the object.
//
// Notes:
//
static int
TypedObject_getRuntimeObject(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);
	
	DbgScriptTypedObject* newTypObj = allocTypedObject(L);
	
	HRESULT hr = DsTypedObjectGetRuntimeType(hostCtxt, typObj, newTypObj);
	if (FAILED(hr))
	{
		return LuaError(L, "DsTypedObjectGetRuntimeType failed. Error 0x%08x.", hr);
	}
	
	return 1;
}

//------------------------------------------------------------------------------
// Function: TypedObject_getfield
//
// Description:
//
//  Get a field in the object.
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
//  One result: The type name of the TypedObject.
//
// Notes:
//
static int
TypedObject_getfield(lua_State* L)
{
	return getFieldHelper(L, nullptr /* typObj */);
}

//------------------------------------------------------------------------------
// Function: TypedObject_readString
//
// Description:
//
//  Read a string from the target process as a Lua string.
//
// Parameters:
//
//  obj.readString([count]) -> str
//
// Input Stack:
//
//  Param 1 is the typed object. (self)
//  Param 2: count. [opt] Max number of characters to read. -1 means read up to NUL.
//
// Returns:
//
//  One result: The type name of the TypedObject.
//
// Notes:
//
static int
TypedObject_readString(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);
	
	checkTypedData(L, typObj);

	const UINT64 addr = typObj->TypedData.Offset;
	const int count = (int)luaL_optinteger(L, 2, -1 /* default val */);
	return LuaReadString(L, addr, count);
}

//------------------------------------------------------------------------------
// Function: TypedObject_readWideString
//
// Synopsis:
//
//  obj.readWideString([count]) -> string
//
// Description:
//
//  Read a string from the target process as a Lua string.
//
// Input Stack:
//
//  Param 1 is the typed object. (self)
//  Param 2: count. [opt] Max number of characters to read. -1 means read up to NUL.
//
// Returns:
//
//  One result: The type name of the TypedObject.
//
// Notes:
//
static int
TypedObject_readWideString(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);
	
	checkTypedData(L, typObj);
	
	const UINT64 addr = typObj->TypedData.Offset;
	const int count = (int)luaL_optinteger(L, 2, -1 /* default val */);
	return LuaReadWideString(L, addr, count);
}

//------------------------------------------------------------------------------
// Function: TypedObject_readBytes
//
// Description:
//
//  Read a block of 'count' bytes from the target process from this object's
//  address.
//
// Parameters:
//
//  obj.readBytes(count) -> string
//
// Input Stack:
//
//  Param 1 is the typed object. (self)
//  Param 2: count of bytes to read.
//
// Returns:
//
//  One result: Bytes as a Lua string.
//
// Notes:
//
static int
TypedObject_readBytes(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	UINT64 addr = 0;
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	addr = typObj->TypedData.Offset;
	
	const ULONG count = (ULONG)luaL_checkinteger(L, 2);
	return LuaReadBytes(L, addr, count);
}

//------------------------------------------------------------------------------
// Function: TypedObject_deref
//
// Description:
//
//  Dereference a pointer (or array) object and return a new TypedObject
//  of the dereferenced type.
//
// Parameters:
//
//  L - pointer to Lua state.
//
// Input Stack:
//
//  Param 1 is the user datum (TypedObject).
//
// Returns:
//
//  One result: Dereferenced type object.
//
// Notes:
//
static int
TypedObject_deref(lua_State* L)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	// Validate that the first param was 'self'. I.e. a Userdatum of the right
	// type. (Having the right metatable).
	//
	DbgScriptTypedObject* typObj = (DbgScriptTypedObject*)
		luaL_checkudata(L, 1, TYPED_OBJECT_METATABLE);

	// Dereferencing a pointer is the same as getting the zero'th element.
	//
	DEBUG_TYPED_DATA typedData = {0};
	HRESULT hr = DsTypedObjectGetArrayElement(
		hostCtxt,
		typObj,
		0,
		&typedData);
	if (FAILED(hr))
	{
		return LuaError(L, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
	}
	
	// Inherit the same name as the parent. This will push the new user datum on
	// the stack.
	//
	allocSubTypedObject(L, typObj->Name, &typedData);
	
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

// Class Properties.
//
static const LuaClassProperty x_TypedObjectProps[] =
{
	// Name   Getter               Setter
	// -----------------------------------------------------------
	{ "name", TypedObject_getname, nullptr },
	{ "size", TypedObject_getsize, nullptr },
	{ "type", TypedObject_gettype, nullptr },
	{ "module", TypedObject_getmodule, nullptr },
	{ "value", TypedObject_getvalue, nullptr },
	{ "address", TypedObject_getaddress, nullptr },
	{ "deref", TypedObject_deref, nullptr },
};

// Instance methods.
//
static const luaL_Reg g_typedObjectMethods[] =
{
	{"__index", TypedObject_index},  // indexer. Serves array, field and property access.
	{"__len", TypedObject_len},  // Length of array, if valid.
	
	// Explicit field access, in case a property hides a field with the same
	// name. 'f' and 'field' are aliases.
	//
	{"f", TypedObject_getfield},
	{"field", TypedObject_getfield},

	// Downcasts an object to its real type based on its vtable.
	//
	{"getRuntimeObject", TypedObject_getRuntimeObject},

	{"readString", TypedObject_readString},
	{"readWideString", TypedObject_readWideString},

	{"readBytes", TypedObject_readBytes},
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

