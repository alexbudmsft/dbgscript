#include "common.h"
#include "process.h"

static VALUE
DbgScript_read_ptr(
	_In_ VALUE /* self */)
{
	return Qnil;
}

void
Init_DbgScript()
{
	VALUE module = rb_define_module("DbgScript");

	rb_define_module_function(
		module, "read_ptr", (RUBYMETHOD)DbgScript_read_ptr, 0);
}

