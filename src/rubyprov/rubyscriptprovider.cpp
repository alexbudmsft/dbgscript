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
#include <crtdbg.h>
#include <iscriptprovider.h>

// Ruby modules and classes.
//
#include "dbgscript.h"
#include "thread.h"
#include "stackframe.h"
#include "typedobject.h"

class CRubyScriptProvider : public IScriptProvider
{
public:
	CRubyScriptProvider();

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

	_CrtMemState MemStateBefore;

	_CrtMemState MemStateAfter;

	_CrtMemState MemStateDiff;
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
	return StartVM();
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
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	IDebugControl* ctrl = hostCtxt->DebugControl;

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

	// First, flush any remaining buffer so the traceback follows it.
	//
	UtilFlushMessageBuffer(hostCtxt);

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
	rb_load(name, 0 /* wrap */);
	return Qnil;
}

static VALUE
runStringGuarded(
	_In_ const char* str)
{
	return rb_eval_string(str);
}

_Check_return_ HRESULT
CRubyScriptProvider::Run(
	_In_ int argc,
	_In_ WCHAR** argv)
{
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
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

	// Convert to ANSI to satisfy Ruby.
	//
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
	_In_z_ const char* scriptString)
{
	// Host ensures string is not empty.
	//
	assert(*scriptString);
	
	// Invoke the script in a "begin..rescue..end" block. (I.e. try/catch in
	// other languages).
	//
	// NOTE: rb_rescue only filters for StandardError exceptions (and subclasses).
	// This does NOT include LoadError. Thus we must use rb_rescue2.
	//
	rb_rescue2(
		RUBY_METHOD_FUNC(runStringGuarded),
		(VALUE)scriptString,
		RUBY_METHOD_FUNC(topLevelExceptionHandler),
		Qnil,
		rb_eException,
		0 /* sentinel */);
	
	return S_OK;
}

extern "C" void Init_ext(void);
extern "C" void Init_enc(void);

static void
lockdownRuby()
{
	//
	// Basic lockdown.
	//
	// Prevent Kernel#exit et al.
	//
	rb_undef_method(rb_mKernel, "exit");
	rb_undef_method(rb_mKernel, "exit!");

#ifdef LOCKDOWN
	// Extensive lockdown for LOCKDOWN build.
	//
	rb_undef_method(rb_mKernel, "open");

	rb_define_global_const("File", Qundef);
#endif
}

_Check_return_ HRESULT
CRubyScriptProvider::StartVM()
{
	// These are populated by Ruby by are unused by us.
	//
	int argc = 0;
	char **argv = nullptr;

	HRESULT hr = S_OK;

	_CrtMemCheckpoint(&MemStateBefore);

	rb_init_global_heap();
	
	// Ruby does command line parsing for us. We're not going to use its output
	// though.
	//
	ruby_sysinit(&argc, &argv);
	int ret = ruby_setup();
	if (ret != 0)
	{
		hr = E_FAIL;  // TODO: Better error.
		goto exit;
	}

	ruby_init_loadpath();
	Init_enc();
	Init_ext();

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
	
	// Initialize StackFrame class.
	//
	Init_TypedObject();

	lockdownRuby();

exit:
	return hr;
}

void
CRubyScriptProvider::StopVM()
{
	int status = ruby_cleanup(0);
	if (status)
	{
		GetRubyProvGlobals()->HostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_WARNING,
			"Warning: Ruby failed to cleanup: %d.\n", status);
	}

	rb_cleanup_global_heap();
	
	_CrtMemCheckpoint(&MemStateAfter);
	
	_CrtMemDifference(
		&MemStateDiff,
		&MemStateBefore,
		&MemStateAfter);
	
	_CrtMemDumpAllObjectsSince(&MemStateBefore);
}

void 
CRubyScriptProvider::Cleanup()
{
	StopVM();
	
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
