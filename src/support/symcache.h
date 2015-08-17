//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: symcache.h
// @Author: alexbud
//
// Purpose:
//
//  Symbol caching layer.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************
#pragma once

#include <windows.h>
#include <hostcontext.h>

struct ModuleAndTypeId
{
	ULONG TypeId;

	UINT64 ModuleBase;
};

_Check_return_ ModuleAndTypeId*
GetCachedSymbolType(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* sym);
