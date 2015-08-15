//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dbgscript.h
// @Author: alexbud
//
// Purpose:
//
//  DbgScript module for Lua Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

#include "common.h"

int
luaopen_dbgscript(lua_State* L);
