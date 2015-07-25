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
		_In_z_ const char* scriptName) override;

	_Check_return_ void
	Cleanup() override;

private:
};
