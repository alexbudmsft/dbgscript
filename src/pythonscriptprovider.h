#pragma once
#include <windows.h>

class CPythonScriptProvider : public IScriptProvider
{
public:
	CPythonScriptProvider(_In_z_ const char* scriptName);

	_Check_return_ HRESULT
	Init() override;

	_Check_return_ void
	Cleanup() override;

private:
	const char* m_ScriptName;
};
