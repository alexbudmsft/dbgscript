#pragma once
#include <windows.h>

class CPythonScriptProvider : public IScriptProvider
{
public:
	CPythonScriptProvider(_In_z_ const WCHAR* scriptName);

	_Check_return_ HRESULT
	Init() override;

	_Check_return_ void
	Cleanup() override;

private:
	const WCHAR* m_ScriptName;
};
