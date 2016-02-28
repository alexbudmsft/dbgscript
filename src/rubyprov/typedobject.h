//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: typedobject.h
// @Author: alexbud
//
// Purpose:
//
//  Typed Object class for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#pragma once

void
Init_TypedObject();

_Check_return_ VALUE
AllocTypedObject(
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_In_ bool wantPointer);
