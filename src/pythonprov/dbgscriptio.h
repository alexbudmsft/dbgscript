#pragma once

#include "../common.h"
#include <python.h>

struct DbgScriptIOObj;

_Check_return_ bool
InitDbgScriptIOType();

_Check_return_ PyObject*
AllocDbgScriptIOObj();
