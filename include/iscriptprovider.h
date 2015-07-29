#pragma once

#include <windows.h>

struct IScriptProvider
{
	virtual _Check_return_ HRESULT 
	Init() = 0;

	virtual _Check_return_ HRESULT 
	Run(
		_In_z_ const char* szScriptName,
		_In_ int argc,
		_In_z_ WCHAR** argv) = 0;

	virtual _Check_return_ HRESULT
	RunString(
		_In_z_ const char* scriptString) = 0;

	virtual _Check_return_ void
	Cleanup() = 0;
};

IScriptProvider*
CreateScriptProvider();