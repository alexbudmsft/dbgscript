#pragma once

#include <python.h>
#include "../util.h"

// Attribute is read-only.
//
int
SetReadOnlyProperty(
	PyObject* /* self */, PyObject* /* value */, void* /* closure */);

// Helper macro to call in every Python entry point that checks for abort
// and raises a KeyboardInterrupt exception.
//
#define CHECK_ABORT \
	do { \
		if (UtilCheckAbort()) \
		{ \
			PyErr_SetNone(PyExc_KeyboardInterrupt); \
			return nullptr; \
		} \
	} while (0)
