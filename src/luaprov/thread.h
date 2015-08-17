//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: thread.h
// @Author: alexbud
//
// Purpose:
//
//  Thread class for Lua Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

#include "common.h"

int
luaopen_Thread(lua_State* L);

DbgScriptThread*
AllocThreadObject(
	_In_ lua_State* L);
