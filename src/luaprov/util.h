//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.h
// @Author: alexbud
//
// Purpose:
//
//  Lua Provider utilities.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

#include "common.h"

int
LuaError(
	_In_ lua_State* L,
	_In_z_ const char* fmt,
	...);

int
LuaReadBytes(
	_In_ lua_State* L,
	_In_ UINT64 addr,
	_In_ ULONG count);

int
LuaReadWideString(
	_In_ lua_State* L,
	_In_ UINT64 addr,
	_In_ int count);

int
LuaReadString(
	_In_ lua_State* L,
	_In_ UINT64 addr,
	_In_ int count);
