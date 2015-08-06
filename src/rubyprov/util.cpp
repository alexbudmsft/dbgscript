//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: util.cpp
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
#include "common.h"

//------------------------------------------------------------------------------
// Function: LockDownClass
//
// Description:
//
//  Lock down key methods in a class.
//  
// Returns:
//
// Notes:
//
void
LockDownClass(
	_In_ VALUE klass)
{
	// Prevent scripter from instantiating directly.
	//
	// 'rb_undef_method' undefs an *instance* method of the given class,
	// so for 'new' which is a 'class method', we have to get the
	// object's class.
	//
	rb_undef_method(CLASS_OF(klass), "new");
	
	rb_undef_method(klass, "dup");
	rb_undef_method(klass, "clone");
	
}
