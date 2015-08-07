#pragma once

#include <windows.h>
#include <dbgeng.h>
#include "../include/iscriptprovider.h"
#include "../include/hostcontext.h"
#include "../include/dsthread.h"
#include "../include/dsstackframe.h"
#include "../include/dstypedobject.h"

#define STRING_AND_CCH(x) x, _countof(x)

_Check_return_ DbgScriptHostContext*
GetHostContext();
