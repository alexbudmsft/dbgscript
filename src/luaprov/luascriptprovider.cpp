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
#include "dbgscript.h"  // dbgscript module.
#include "typedobject.h"

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
	
private:
	
	lua_State* LuaState;
};

CLuaScriptProvider::CLuaScriptProvider()
{

}

_Check_return_ HRESULT 
CLuaScriptProvider::Init()
{
	return StartVM();
}

void 
luaOutputCb(
	_In_ lua_OutputType out_type,
	_In_ const char* buf)
{
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	hostCtxt->DebugControl->Output(
		out_type == lua_outputNormal ? DEBUG_OUTPUT_NORMAL : DEBUG_OUTPUT_ERROR,
		buf);
}

void
luaInputCb(
	_Out_writes_(len) char* buf,
	_In_ size_t len)
{
	ULONG cchRead = 0;
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;

	// Pretend the buffer is one smaller than it really is so we can stuff a
	// newline in there.
	//
	hostCtxt->DebugControl->Input(buf, (ULONG)len - 1, &cchRead);

	// Append a newline to simulate fgets.
	//
	buf[cchRead - 1] = '\n';
	buf[cchRead] = 0;
}

// Get one char at a time from stdin. Used by io.read() in lua.
//
int
luaGetCCb()
{
	int ret = 0;
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	HRESULT hr = S_OK;
	if (!GetLuaProvGlobals()->ValidInputChars)
	{
		ULONG charsRead = 0;

		// If the input buffer is empty, ask the user for more input.
		//
		hr = hostCtxt->DebugControl->Input(
			GetLuaProvGlobals()->InputBuf,
			_countof(GetLuaProvGlobals()->InputBuf) - 1,
			&charsRead);
		if (FAILED(hr))
		{
			ret = EOF;
			goto exit;
		}

		// Add a newline at the end to simulate a console-style read line.
		// We can always do this because we told Input() that our buffer is
		// smaller than it really is. So we always have at least one extra char
		// after charsRead. The 'charsRead' index points one-past the NUL char.
		//
		// NOTE: NUL termination doesn't even matter here: We're not serving
		// strings. We're serving characters.
		//
		GetLuaProvGlobals()->InputBuf[charsRead - 1] = '\n';

		// NUL terminating just for cleanliness and viewing strings in debugger.
		// Not actually needed.
		//
		GetLuaProvGlobals()->InputBuf[charsRead] = 0;

		// Don't even bother serving back the NUL: The caller is not interested.
		// I.e. charsRead, not charsRead + 1.
		//
		GetLuaProvGlobals()->ValidInputChars = charsRead;

		GetLuaProvGlobals()->NextInputChar = 0;
	}

	// Otherwise serve our input buffer one char at a time.
	//
	ret = GetLuaProvGlobals()->InputBuf[GetLuaProvGlobals()->NextInputChar++];
	GetLuaProvGlobals()->ValidInputChars--;
exit:
	return ret;
}

_Check_return_ HRESULT
CLuaScriptProvider::Run(
	_In_ int argc,
	_In_ WCHAR** argv)
{
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	WCHAR fullScriptName[MAX_PATH] = {};
	size_t cConverted = 0;
	char ansiScriptFileName[MAX_PATH] = {};

	int i = 0;
	bool debug = false;

	// TODO: Generalize arg processing.
	//
	for (i = 0; i < argc; ++i)
	{
		// Terminate at first non-switch.
		//
		if (argv[i][0] != L'-')
		{
			break;
		}

		// We have a switch.
		//
		if (!wcscmp(argv[i], L"-d"))
		{
			debug = true;
		}
		else
		{
			hr = E_INVALIDARG;
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: Unknown switch '%ls'.\n", argv[i]);
			goto exit;
		}		
	}

	// The left over args go to the script itself.
	//
	WCHAR** argsForScript = &argv[i];
	int cArgsForScript = argc - i;
	assert(cArgsForScript >= 0);
	
	if (!cArgsForScript)
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
		argsForScript[0],
		STRING_AND_CCH(fullScriptName));
	if (FAILED(hr))
	{
		goto exit;
	}

	// Replace first arg with the fully qualified path.
	//
	argsForScript[0] = fullScriptName;

	// Convert to ANSI.
	//
	errno_t err = wcstombs_s(
		&cConverted,
		ansiScriptFileName,
		sizeof ansiScriptFileName,
		fullScriptName,
		sizeof ansiScriptFileName - 1);
	if (err)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to convert wide string to ANSI: %d.\n", err);
		hr = E_FAIL;
		goto exit;
	}

	err = luaL_loadfile(LuaState, ansiScriptFileName);
	if (err)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Lua compile error %d: %s\n", err, lua_tostring(LuaState, -1));
		
		// Pop the error message from the stack.
		//
		lua_pop(LuaState, 1);
		hr = E_FAIL;
		goto exit;
	}

	// Push 'debug' [module] table on top of stack (-1)
	//
	lua_getglobal(LuaState, "debug");

	// Push 'traceback'/'debug' function on top of stack (-1).
	//
	// 'debug' [module] is at -2.
	//
	// The debug function acts as a JIT debugger if an exception is thrown.
	// It breaks into a REPL allowing arbitrary lua statements to be executed.
	//
	lua_getfield(LuaState, -1, debug ? "debug" : "traceback");

	// Remove 'debug' table.
	//
	lua_remove(LuaState, -2);

	// Now -2 is the function we want to call. (The user's compiled script chunk)
	//
	// Move 'traceback' below the user function.
	//
	lua_insert(LuaState, -2);

	// TODO: Establish 'args' global so script can have params.
	//

	// Call what's on the top of the stack.
	//
	err = lua_pcall(LuaState, 0, 0, -2 /* position of debug.traceback */);
	
	if (err)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Lua runtime error %d: %s\n", err, lua_tostring(LuaState, -1));

		// Pop the error message from the stack.
		//
		lua_pop(LuaState, 1);
		hr = E_FAIL;
		goto exit;
	}
	
exit:
	return hr;
}

_Check_return_ HRESULT
CLuaScriptProvider::RunString(
	_In_z_ const char* /* scriptString */)
{
	// TODO
	return S_OK;
}

_Check_return_ HRESULT
CLuaScriptProvider::StartVM()
{
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetLuaProvGlobals()->HostCtxt;
	lua_StdioRedir redir = { 0 };
	LuaState = luaL_newstate();
	if (!LuaState)
	{
		hr = E_OUTOFMEMORY;
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: luaL_newstate failed.\n");
		goto exit;
	}

	// Redirect stdio.
	//
	redir.cb_output = luaOutputCb;
	redir.cb_input = luaInputCb;
	redir.cb_getc = luaGetCCb;

	// This is the only global object. But no different from the global
	// stdout/in/err that Lua uses already.
	//
	lua_set_stdio_callbacks(&redir);

	// Open standard libs.
	//
	luaL_openlibs(LuaState);

	// Open dbgscript module.
	//
	luaL_requiref(LuaState, "dbgscript", luaopen_dbgscript, 1 /* set global */);

	// Open TypedObject class.
	//
	luaL_requiref(LuaState, "TypedObject", luaopen_TypedObject, 0 /* set global */);

exit:
	return hr;
}

void
CLuaScriptProvider::StopVM()
{
	if (LuaState)
	{
		lua_close(LuaState);
		LuaState = nullptr;
	}
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
