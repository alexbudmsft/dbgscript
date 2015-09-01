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

#define ERR_SCRIPT_NOT_FOUND \
	"Error: Script file not found in any of the search paths.\n"

#define ERR_FAILED_CREATE_SYM_GRP \
	"Error: Failed to create symbol group. Error 0x%08x.\n"

#define ERR_FAILED_GET_NUM_SYM \
	"Error: Failed to get number of symbols. Error 0x%08x.\n"

#define ERR_FAILED_GET_TEB \
	"Error: Failed to get TEB. Error 0x%08x.\n"

#define ERR_FAILED_GET_SYM_ENTRY_INFO \
	"Error: Failed to get symbol entry information. Error 0x%08x.\n"

#define ERR_FAILED_GET_SYM_NAME \
	"Error: Failed to get symbol name. Error 0x%08x.\n"

#define ERR_FAILED_GET_TYPE_NAME \
	"Error: Failed to get type name.\n"

#define ERR_FAILED_GET_TYPE_ID \
	"Error: Failed to get type id for type '%s'. Error 0x%08x.\n"

#define ERR_FAILED_INSTALL_OUTPUT_CB \
	"Error: Failed to install output callback. Error 0x%08x.\n"
	
#define ERR_FAILED_GET_NUM_THREADS \
	"Error: Failed to get number of threads. Error 0x%08x.\n"
	
#define ERR_FAILED_GET_THREAD_IDS \
	"Error: Failed to get thread IDs. Error 0x%08x.\n"

#define ERR_FAILED_GET_MODULE_NAME \
	"Error: Failed to get module name.\n"
	
#define ERR_FAILED_GET_NAME_BY_OFFSET \
	"Error: Failed to get name from offset %p. Error 0x%08x.\n"

#define ERR_FAILED_READ_PTR \
	"Error: Failed to read pointer from offset %p. Error 0x%08x.\n"

#define ERR_NO_VTABLE \
	"Error: Object at %p has no vtable.\n"

