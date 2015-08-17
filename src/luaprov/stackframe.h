//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: stackframe.h
// @Author: alexbud
//
// Purpose:
//
//  StackFrame class for Lua Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

#include "common.h"

int
luaopen_StackFrame(lua_State* L);

DbgScriptStackFrame*
AllocStackFrameObject(
	_In_ lua_State* L,
	_In_ int thdIdx);

