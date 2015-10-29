//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: cmdline.h
// @Author: alexbud
//
// Purpose:
//
//  Command line processing.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

// ParsedArgs - structure to hold parsed argument values.
//
struct ParsedArgs
{
	// TimeRun - was -t provided? Run will be timed and reported.
	//
	bool TimeRun;

	// LangId - language id provided.
	//
	WCHAR LangId[MAX_LANG_ID]; // -l <lang>

	// RemainingArgs - Remaining string after all host switches were processed
	// and consumed.
	//
	const char* RemainingArgs;
};

_Check_return_ HRESULT
ParseArgs(
	_In_z_ char* args,
	_Out_ ParsedArgs* parsedArgs);
