//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dserrors.h
// @Author: alexbud
//
// Purpose:
//
//  DbgScript Errors.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************
#pragma once

#define ERR_EXEC_CMD_FAILED_FMT \
	"Error: Failed to execute command '%s'. Error 0x%08x.\n"

#define ERR_FAILED_GET_SYM_SCOPE \
	"Error: Failed to get symbol scope. Error 0x%08x.\n"
	
#define ERR_FAILED_SET_SYM_SCOPE \
	"Error: Failed to set symbol scope. Error 0x%08x.\n"