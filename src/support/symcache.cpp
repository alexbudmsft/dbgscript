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
#include "../common.h"
#include <map>

// Key is symbol name.
//
typedef std::map<std::string, ModuleAndTypeId> SymCacheMapT;

// Key is the module base, value is cached copy of module name.
//
typedef std::map<UINT64, std::string> ModuleCacheMapT;

// Key is module/type-id, value is cached copy of type name.
//
typedef std::map<ModuleAndTypeId, std::string> TypeNameCacheMapT;

static SymCacheMapT s_SymCache;
static ModuleCacheMapT s_ModCache;
static TypeNameCacheMapT s_TypeNameCache;

//------------------------------------------------------------------------------
// Function: GetCachedSymbolType
//
// Description:
//
//  Given a name, returns the symbol's type ID and module base.
//
// Parameters:
//
// Returns:
//
//  ModuleAndTypeId.
//
// Notes:
//
_Check_return_ ModuleAndTypeId*
GetCachedSymbolType(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* sym)
{
	ModuleAndTypeId tmp = {0};
	SymCacheMapT::iterator it = s_SymCache.find(sym);
	if (it != s_SymCache.end())
	{
		// Found.
		//
		return &it->second;
	}

	// Not found. Lookup from source of truth.
	//
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

//------------------------------------------------------------------------------
// Function: GetCachedModuleName
//
// Description:
//
//  Given a module base, returns the module name.
//
// Parameters:
//
// Returns:
//
//  Module name.
//
// Notes:
//
//  Do not free the returned pointer!
//
_Check_return_ const char*
GetCachedModuleName(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ UINT64 modBase)
{
	char modName[MAX_MODULE_NAME_LEN] = {};
	ModuleCacheMapT::iterator it = s_ModCache.find(modBase);
	if (it != s_ModCache.end())
	{
		// Found.
		//
		return it->second.c_str();
	}

	// Not found. Populate cache.
	//
	HRESULT hr = hostCtxt->DebugSymbols->GetModuleNames(
		DEBUG_ANY_ID,
		modBase,
		nullptr,  // imageNameBuffer
		0,		  // imageNameBufferSize
		nullptr,  // imageNameSize
		STRING_AND_CCH(modName),
		nullptr,  // moduleNameSize
		nullptr,
		0,
		nullptr);
	if (FAILED(hr))
	{
		return nullptr;
	}

	std::string& str = s_ModCache[modBase];
	str = modName;

	return str.c_str();
}

//------------------------------------------------------------------------------
// Function: GetCachedTypeName
//
// Description:
//
//  Given a module and type id, returns the cached type name.
//
// Parameters:
//
// Returns:
//
//  Type name.
//
// Notes:
//
//  Do not free the returned pointer!
//
_Check_return_ const char*
GetCachedTypeName(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const ModuleAndTypeId& modAndTypeId)
{
	char typeName[MAX_SYMBOL_NAME_LEN] = {};
	TypeNameCacheMapT::iterator it = s_TypeNameCache.find(modAndTypeId);
	if (it != s_TypeNameCache.end())
	{
		// Found.
		//
		return it->second.c_str();
	}

	// Not found. Populate cache.
	//
	HRESULT hr = hostCtxt->DebugSymbols->GetTypeName(
		modAndTypeId.ModuleBase,
		modAndTypeId.TypeId,
		STRING_AND_CCH(typeName),
		nullptr);
	if (FAILED(hr))
	{
		return nullptr;
	}

	std::string& str = s_TypeNameCache[modAndTypeId];
	str = typeName;

	return str.c_str();
}


