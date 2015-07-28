#pragma once

#include <python.h>

// Attribute is read-only.
//
int
SetReadOnlyProperty(
	PyObject* /* self */, PyObject* /* value */, void* /* closure */);
