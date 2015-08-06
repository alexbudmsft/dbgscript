#pragma once

#include <windows.h>
#include <dbgeng.h>
#include "../include/iscriptprovider.h"
#include "../include/hostcontext.h"
#include "../include/dsthread.h"
#include "../include/dsstackframe.h"

#define STRING_AND_CCH(x) x, _countof(x)

// Limits of our extension.
//
const int MAX_MODULE_NAME_LEN = 256;
const int MAX_SYMBOL_NAME_LEN = 512;

_Check_return_ DbgScriptHostContext*
GetHostContext();
