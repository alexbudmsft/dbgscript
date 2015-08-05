//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dsthread.h
// @Author: alexbud
//
// Purpose:
//
//  DbgScript Thread Object.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************
#pragma once

// DbgScriptThread - Object that models a debugger thread.
//
struct DbgScriptThread
{
	// Engine thread ID.
	//
	ULONG EngineId;

	// System Thread ID.
	//
	ULONG ThreadId;
};
