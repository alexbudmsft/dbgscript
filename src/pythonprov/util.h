#pragma once

#include <python.h>
#include "../support/util.h"

// Attribute is read-only.
//
int
SetReadOnlyProperty(
	PyObject* /* self */, PyObject* /* value */, void* /* closure */);

void
RedirectStdIO(
	_In_ PyObject* obj);

// Helper macro to call in every Python entry point that checks for abort
// and raises a KeyboardInterrupt exception.
//
#define CHECK_ABORT(ctxt) \
	do { \
		if (UtilCheckAbort(ctxt)) \
		{ \
			PyErr_SetNone(PyExc_KeyboardInterrupt); \
			return nullptr; \
		} \
	} while (0)
