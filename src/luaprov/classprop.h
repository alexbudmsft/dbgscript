//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: classprop.h
// @Author: alexbud
//
// Purpose:
//
//  Class Property machinery for Lua.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

#include "common.h"

// LuaClassProperty - Registration structure to be transformed into Lua class
// properties.
//
// Simulates Python/C# attributes/properties with getters/setters.
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

int
LuaClassPropIndexer(lua_State* L);

void
LuaSetProperties(
	_In_ lua_State* L,
	_In_reads_(count) const LuaClassProperty* props,
	_In_ int count);
