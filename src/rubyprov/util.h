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

VALUE
RbReadBytes(
	_In_ UINT64 addr,
	_In_ VALUE count);
