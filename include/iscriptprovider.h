#pragma once

#include <windows.h>

struct IScriptProvider
{
	virtual _Check_return_ HRESULT Init() = 0;
	virtual _Check_return_ void Cleanup() = 0;
};

IScriptProvider*
CreateScriptProvider(
	_In_z_ const WCHAR* scriptName);