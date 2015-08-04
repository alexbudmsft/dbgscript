#pragma warning(push)
#pragma warning(disable: 4510 4512 4610 4100)
#include <ruby.h>
#pragma warning(pop)

#include <iscriptprovider.h>

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
customWrite(
	_In_ VALUE /*self*/,
	_In_ VALUE input)
{
	const char* str = StringValueCStr(input);
	str;
	// TODO: Global IDebugControl->Output.
	//
	return Qnil;
}

_Check_return_ HRESULT 
CRubyScriptProvider::Init()
{
	// Fake argc/argv.
	//
	int argc = 1;
	char* args[] = { "embed" };
	char **argv = args;

	HRESULT hr = S_OK;
	ruby_sysinit(&argc, &argv);
	RUBY_INIT_STACK;
	int ret = ruby_setup();
	if (ret != 0)
	{
		hr = E_FAIL;  // TODO: Better error.
		goto exit;
	}

	// TODO: Verify we can access the entire ruby standard library. Same goes for python when we implement it.
	//
	ruby_init_loadpath();

	// Redirect all writes/prints to our custom routine.
	//
	VALUE out = rb_class_new_instance(0, 0, rb_cObject);
	rb_define_singleton_method(
		out,
		"write",
		RUBY_METHOD_FUNC(customWrite),
		1 /* numParams */);

	// Set the ruby stdout/stderr to use our newly-created object.
	//
	rb_stdout = out;
	rb_stderr = out;
exit:
	return hr;
}

_Check_return_ HRESULT
CRubyScriptProvider::Run(
	_In_ int argc,
	_In_ WCHAR** argv)
{
	return S_OK;
}

_Check_return_ HRESULT
CRubyScriptProvider::RunString(
	_In_z_ const char* scriptString)
{
	return S_OK;
}

_Check_return_ void 
CRubyScriptProvider::Cleanup()
{
	ruby_cleanup(0);
}

/* Run the script.

// TODO: set the logical script name for ruby's $0 variable, based on the filename we're running.
//
// ruby_script("new name")
//
void* node = rb_load_file(args);
if (0 != ruby_exec_node(node))
{
}


*/