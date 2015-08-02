#include "symcache.h"
#include <map>

typedef std::map<std::string, ModuleAndTypeId> SymCacheMapT;

static SymCacheMapT s_SymCache;

_Check_return_ ModuleAndTypeId*
GetCachedSymbolType(
	_In_z_ const char* sym)
{
	SymCacheMapT::iterator it = s_SymCache.find(sym);
	if (it != s_SymCache.end())
	{
		return &it->second;
	}

	ModuleAndTypeId& info = s_SymCache[sym];

	HRESULT hr = GetDllGlobals()->DebugSymbols->GetSymbolTypeId(
		sym,
		&info.TypeId,
		&info.ModuleBase);
	if (FAILED(hr))
	{
		return nullptr;
	}

	return &info;
}