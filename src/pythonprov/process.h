#pragma once
#include "../common.h"
#include <python.h>

_Check_return_ bool
InitProcessType();

_Check_return_ PyObject*
AllocProcessObj();