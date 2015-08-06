//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: rubyscriptprovider.cpp
// @Author: alexbud
//
// Purpose:
//
//  Ruby Script Provider primary module.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "common.h"

#include <iscriptprovider.h>

// Ruby modules and classes.
//
#include "dbgscript.h"
#include "thread.h"
#include "stackframe.h"

class CRubyScriptProvider : public IScriptProvider
{
public:
	CRubyScriptProvider();

	_Check_return_ HRESULT
	Init() override;

	_Check_return_ HRESULT
	Run(
		_In_ int argc,
		_In_ WCHAR** argv) override;

	_Check_return_ HRESULT
	RunString(
		_In_z_ const char* scriptString) override;

	_Check_return_ void
	Cleanup() override;
};

CRubyScriptProvider::CRubyScriptProvider()
{

}

static VALUE
DbgScriptStdIO_gets(
	_In_ int /*argc*/,
	_In_ VALUE * /*argv*/,
	_In_ VALUE /*self*/)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	// IO.gets supports a bunch of different parameters (see 'rb_io_gets_m').
	//
	// Maybe in the future we'll support them, but now now, just handle the
	// common use-case: no args.
	//
	// I.e. read one line from the debugger.
	//
	char buf[4096] = { 0 };
	ULONG actualLen = 0;

	hostCtxt->DebugControl->Input(
		buf,
		_countof(buf),
		&actualLen);

	return rb_str_new2(buf);
}

static VALUE
DbgScriptStdIO_write(
	_In_ VALUE /*self*/,
	_In_ VALUE input)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	const char* str = StringValueCStr(input);

	if (hostCtxt->IsBuffering > 0)
	{
		UtilBufferOutput(hostCtxt, str, strlen(str));
	}
	else
	{
		hostCtxt->DebugControl->Output(DEBUG_OUTPUT_NORMAL, "%s", str);
	}

	return Qnil;
}

_Check_return_ HRESULT 
CRubyScriptProvider::Init()
{
	int argc = 0;
	char **argv = nullptr;

	HRESULT hr = S_OK;

	// Ruby does command line parsing for us. We're not going to use its output
	// though.
	//
	ruby_sysinit(&argc, &argv);
	RUBY_INIT_STACK;
	int ret = ruby_setup();
	if (ret != 0)
	{
		hr = E_FAIL;  // TODO: Better error.
		goto exit;
	}

	ruby_init_loadpath();

	// Redirect all writes/prints/readlines to our custom routines.
	//
	rb_define_singleton_method(
		rb_stdout,
		"write",
		RUBY_METHOD_FUNC(DbgScriptStdIO_write),
		1 /* numParams */);

	rb_define_singleton_method(
		rb_stderr,
		"write",
		RUBY_METHOD_FUNC(DbgScriptStdIO_write),
		1 /* numParams */);

	rb_define_singleton_method(
		rb_stdin,
		"gets",
		RUBY_METHOD_FUNC(DbgScriptStdIO_gets),
		-1 /* numParams */);

	// Initialize DbgScript module.
	//
	Init_DbgScript();

	// Initialize Thread class.
	//
	Init_Thread();

	// Initialize StackFrame class.
	//
	Init_StackFrame();

	// Prevent Kernel#exit et al.
	//
	rb_undef_method(rb_mKernel, "exit");
	rb_undef_method(rb_mKernel, "exit!");

exit:
	return hr;
}

struct NoArgMethodInfo
{
	VALUE Object;
	ID Method;
};

static VALUE 
noArgMethodWrapper(
	_In_ VALUE args)
{
	NoArgMethodInfo* info = (NoArgMethodInfo*)args;
	return rb_funcall(info->Object, info->Method, 0);
}

// Catches all exceptions.
//
static VALUE 
topLevelExceptionHandler(
	_In_ VALUE /*args*/,
	_In_ VALUE exc)
{
	IDebugControl* ctrl = GetRubyProvGlobals()->HostCtxt->DebugControl;

	NoArgMethodInfo info = { exc, rb_intern("message") };

	int status = 0;
	VALUE message = rb_protect(noArgMethodWrapper, (VALUE)&info, &status);
	if (status)
	{
		ctrl->Output(
			DEBUG_OUTPUT_ERROR, "Exception occurred while trying to obtain message for unhandled exception! %d\n",
			status);
		goto exit;
	}

	// Assumes a 'simple' null-terminated string.
	//
	ctrl->Output(
		DEBUG_OUTPUT_ERROR, "Unhandled exception: %s: %s\n",
		rb_obj_classname(exc), RSTRING_PTR(message));

	// TODO: Protect.
	//
	VALUE stackTrace = rb_funcall(exc, rb_intern("backtrace"), 0);
	const int stackLen = RARRAY_LEN(stackTrace);

	ctrl->Output(
		DEBUG_OUTPUT_ERROR, "Stack trace:\n");
	for (int i = 0; i < stackLen; ++i)
	{
		VALUE frame = rb_ary_entry(stackTrace, i);

		// Normally we can't assume a Ruby string is null terminated or
		// doesn't contain embedded NULLs. In this case however, the string
		// is a stack frame, which should be a "simple" string.
		//
		const char* frameStr = RSTRING_PTR(frame);

		ctrl->Output(
			DEBUG_OUTPUT_ERROR, "  %s\n", frameStr);
	}
exit:
	return Qnil;
}

static _Check_return_ HRESULT
narrowArgvFromWide(
	_In_ int argc,
	_In_ WCHAR** argv,
	_Outptr_ char*** narrowArgv)
{
	HRESULT hr = S_OK;
	*narrowArgv = new char*[argc];
	if (!*narrowArgv)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	for (int i = 0; i < argc; ++i)
	{
		size_t cConverted = 0;

		const size_t cbAnsiArg = wcslen(argv[i]) + 1;
		char* ansiArg = new char[cbAnsiArg];
		if (!ansiArg)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		// Convert to ANSI.
		//
		errno_t err = wcstombs_s(
			&cConverted, ansiArg, cbAnsiArg, argv[i], cbAnsiArg - 1);
		if (err)
		{
			GetRubyProvGlobals()->HostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: Failed to convert wide string to ANSI: %d.\n", err);
			hr = E_FAIL;
			goto exit;
		}

		(*narrowArgv)[i] = ansiArg;
	}

exit:
	return hr;
}

static VALUE
runScriptGuarded(VALUE name)
{
	rb_load(name, 0);
	return Qnil;
}

_Check_return_ HRESULT
CRubyScriptProvider::Run(
	_In_ int argc,
	_In_ WCHAR** argv)
{
	HRESULT hr = S_OK;
	char** narrowArgv = nullptr;
	hr = narrowArgvFromWide(argc, argv, &narrowArgv);
	if (FAILED(hr))
	{
		goto exit;
	}

	ruby_script(narrowArgv[0]);

	// Set up argv for the script.
	//
	ruby_set_argv(argc, narrowArgv);

	// Invoke the script in a "begin..rescue..end" block. (I.e. try/catch in
	// other languages).
	//
	// NOTE: rb_rescue only filters for StandardError exceptions (and subclasses).
	// This does NOT include LoadError. Thus we must use rb_rescue2.
	//
	rb_rescue2(
		RUBY_METHOD_FUNC(runScriptGuarded),
		rb_str_new2(narrowArgv[0]),
		RUBY_METHOD_FUNC(topLevelExceptionHandler),
		Qnil,
		rb_eException,
		0 /* sentinel */);

exit:
	return hr;
}

_Check_return_ HRESULT
CRubyScriptProvider::RunString(
	_In_z_ const char* /*scriptString*/)
{
	// TODO
	//
	ruby_script("<embed>");
	return S_OK;
}

_Check_return_ void 
CRubyScriptProvider::Cleanup()
{
	int status = ruby_cleanup(0);
	if (status)
	{
		GetRubyProvGlobals()->HostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_WARNING,
			"Warning: Ruby failed to cleanup: %d.\n", status);
	}

	delete this;
}

_Check_return_ DLLEXPORT HRESULT
ScriptProviderInit(
	_In_ DbgScriptHostContext* hostCtxt)
{
	GetRubyProvGlobals()->HostCtxt = hostCtxt;
	return S_OK;
}

_Check_return_ DLLEXPORT void
ScriptProviderCleanup()
{
}

DLLEXPORT IScriptProvider*
ScriptProviderCreate()
{
	return new CRubyScriptProvider;
}
