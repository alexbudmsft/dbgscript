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

// Replace sys.stdout and sys.stderr with 'obj'.
//
void
RedirectStdIO(
	_In_ PyObject* obj)
{
	PySys_SetObject("stdout", obj);
	PySys_SetObject("__stdout__", obj);
	PySys_SetObject("stderr", obj);
	PySys_SetObject("__stderr__", obj);
	PySys_SetObject("stdin", obj);
	PySys_SetObject("__stdin__", obj);
}

