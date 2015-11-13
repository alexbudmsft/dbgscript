//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.h
// @Author: alexbud
//
// Purpose:
//
//  Utilities for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#pragma once

void
LockDownClass(
	_In_ VALUE klass);

_Check_return_ VALUE
RbReadBytes(
	_In_ UINT64 addr,
	_In_ VALUE count);

_Check_return_ VALUE
RbReadString(
	_In_ UINT64 addr,
	_In_ int count);

_Check_return_ VALUE
RbReadWideString(
	_In_ UINT64 addr,
	_In_ int count);
