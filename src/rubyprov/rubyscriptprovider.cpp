#pragma warning(push)
#pragma warning(disable: 4510 4512 4610 4100)
#include <ruby.h>
#pragma warning(pop)

#include "../include/iscriptprovider.h"

class CRubyScriptProvider : public IScriptProvider
{
	_Check_return_ HRESULT 
	Init() override;

	_Check_return_ HRESULT 
	Cleanup() override;
};

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
	HRESULT hr = S_OK;
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