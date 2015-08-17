//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: symcache.cpp
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

#include "symcache.h"
#include <map>

typedef std::map<std::string, ModuleAndTypeId> SymCacheMapT;

static SymCacheMapT s_SymCache;

_Check_return_ ModuleAndTypeId*
GetCachedSymbolType(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* sym)
{
	ModuleAndTypeId tmp = {0};
	SymCacheMapT::iterator it = s_SymCache.find(sym);
	if (it != s_SymCache.end())
	{
		return &it->second;
	}

	HRESULT hr = hostCtxt->DebugSymbols->GetSymbolTypeId(
		sym,
		&tmp.TypeId,
		&tmp.ModuleBase);
	if (FAILED(hr))
	{
		return nullptr;
	}

	ModuleAndTypeId& info = s_SymCache[sym];
	info = tmp;

	return &info;
}
