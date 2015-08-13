//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: luascriptprovider.cpp
// @Author: alexbud
//
// Purpose:
//
//  Lua Script Provider primary module.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "common.h"

#include <iscriptprovider.h>
#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>

// Lua modules and classes.
//
// ...

// CLuaScriptProvider - Interface implementation for Lua provider.
//
class CLuaScriptProvider : public IScriptProvider
{
public:
	CLuaScriptProvider();

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

CLuaScriptProvider::CLuaScriptProvider()
{

}

_Check_return_ HRESULT 
CLuaScriptProvider::Init()
{
	return StartVM();
}

_Check_return_ HRESULT
CLuaScriptProvider::Run(
	_In_ int argc,
	_In_ WCHAR** argv)
{
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	WCHAR fullScriptName[MAX_PATH] = {};
	
	if (!argc)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: No arguments given.\n");
		hr = E_INVALIDARG;
		goto exit;
	}

	// To to lookup the script in the registered !scriptpath's.
	//
	hr = UtilFindScriptFile(
		hostCtxt,
		argv[0],
		STRING_AND_CCH(fullScriptName));
	if (FAILED(hr))
	{
		goto exit;
	}

	// Replace argv[0] with the fully qualified path.
	//
	argv[0] = fullScriptName;

	// TODO.
	//
exit:
	return hr;
}

_Check_return_ HRESULT
CLuaScriptProvider::RunString(
	_In_z_ const char* /* scriptString */)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	HRESULT hr = S_OK;
	lua_State* L = nullptr;

	L = luaL_newstate();

	luaL_openlibs(L);

	int err = luaL_loadstring(L, "print(1)");
	if (err)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: luaL_loadstring failed: %d\n", err);
		hr = E_FAIL;
		goto exit;
	}

	err = lua_pcall(L, 0, 0, 0);
	if (err)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: lua_pcall failed: %d\n", err);
		hr = E_FAIL;
		goto exit;
	}
	
exit:
	if (L)
	{
		lua_close(L);
		L = nullptr;
	}
	return S_OK;
}

_Check_return_ HRESULT
CLuaScriptProvider::StartVM()
{

	return S_OK;
}

void
CLuaScriptProvider::StopVM()
{

}

void 
CLuaScriptProvider::Cleanup()
{
	StopVM();
	
	delete this;
}

//
// Exports from the DLL.
//

_Check_return_ DLLEXPORT HRESULT
ScriptProviderInit(
	_In_ DbgScriptHostContext* hostCtxt)
{
	GetLuaProvGlobals()->HostCtxt = hostCtxt;
	return S_OK;
}

_Check_return_ DLLEXPORT void
ScriptProviderCleanup()
{
}

DLLEXPORT IScriptProvider*
ScriptProviderCreate()
{
	return new CLuaScriptProvider;
}
