#pragma once
#include <windows.h>

#include <iscriptprovider.h>

class CPythonScriptProvider : public IScriptProvider
{
public:
	CPythonScriptProvider();

	_Check_return_ HRESULT
	Init() override;
	
	_Check_return_ HRESULT
	StartVM() override;

	void
	StopVM() override;

	_Check_return_ HRESULT
	Run(
		_In_ int argc,
		_In_ WCHAR** argv) override;

	_Check_return_ HRESULT
	RunString(
		_In_z_ const char* scriptString) override;

	void
	Cleanup() override;
};
