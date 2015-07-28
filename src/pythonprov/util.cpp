#include "util.h"
#include <python.h>

// Attribute is read-only. TODO: Share this out to other modules.
//
int
SetReadOnlyProperty(
	PyObject* /* self */, PyObject* /* value */, void* /* closure */)
{
	PyErr_SetString(PyExc_AttributeError, "readonly attribute");
	return -1;
}

