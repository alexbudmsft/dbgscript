//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: typedobject.h
// @Author: alexbud
//
// Purpose:
//
//  TypedObject class for Lua Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

#include "common.h"

int
luaopen_TypedObject(lua_State* L);

void
AllocTypedObject(
	_In_ lua_State* L,
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_z_ const char* type,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress);
