#pragma once
#include "../common.h"
#include <python.h>

static const char* x_DbgScriptModuleName = "dbgscript";

PyMODINIT_FUNC
PyInit_dbgscript();