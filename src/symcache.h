#pragma once

#include "common.h"

struct ModuleAndTypeId
{
	ULONG TypeId;

	UINT64 ModuleBase;
};

_Check_return_ ModuleAndTypeId*
GetCachedSymbolType(
	_In_z_ const char* sym);