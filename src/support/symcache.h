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

	bool operator<(const ModuleAndTypeId& other) const
	{
		if (ModuleBase != other.ModuleBase)
		{
			return ModuleBase < other.ModuleBase;
		}

		return TypeId < other.TypeId;
	}
};

_Check_return_ ModuleAndTypeId*
GetCachedSymbolType(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* sym);

_Check_return_ const char*
GetCachedModuleName(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 modBase);

_Check_return_ const char*
GetCachedTypeName(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const ModuleAndTypeId& modAndTypeId);
