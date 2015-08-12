//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: thread.h
// @Author: alexbud
//
// Purpose:
//
//  Thread class for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#pragma once

void
Init_Thread();

_Check_return_ VALUE
AllocThreadObj(
	_In_ ULONG engineId,
	_In_ ULONG threadId);

