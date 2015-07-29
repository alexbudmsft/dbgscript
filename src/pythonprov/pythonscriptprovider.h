#pragma once
#include <windows.h>

class CPythonScriptProvider : public IScriptProvider
{
public:
	CPythonScriptProvider();

	_Check_return_ HRESULT
	Init() override;

	_Check_return_ HRESULT
	Run(
		_In_z_ const char* szScriptName,
		_In_ int argc,
		_In_z_ WCHAR** argv) override;

	_Check_return_ HRESULT
	RunString(
		_In_z_ const char* scriptString) override;

	_Check_return_ void
	Cleanup() override;

private:
};
