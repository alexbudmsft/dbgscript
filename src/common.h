//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: common.h
// @Author: alexbud
//
// Purpose:
//
//  DbgScript common header.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#pragma once

//
// Common includes.
//

#include <windows.h>
#include <dbgeng.h>
#include "../include/iscriptprovider.h"
#include "../include/hostcontext.h"
#include "../include/dsthread.h"
#include "../include/dsstackframe.h"
#include "../include/dstypedobject.h"

//
// Useful macros.
//

#define STRING_AND_CCH(x) x, _countof(x)

//
// Constants.
//

const int MAX_LANG_ID = 64;

//
// Global APIs.
//

_Check_return_ DbgScriptHostContext*
GetHostContext();
