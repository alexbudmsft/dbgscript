#pragma once

#include <windows.h>

#define DBG_SCRIPT_VER_MAJ 1
#define DBG_SCRIPT_VER_MIN 0
#define DBG_SCRIPT_VER_BETA 1

#define STR2(x)  #x
#define STR(x) STR2(x)

#ifdef _DEBUG
#define DBG_SCRIPT_FILE_FLAGS VS_FF_DEBUG
#else
#define DBG_SCRIPT_FILE_FLAGS 0
#endif

#define VER_COMPANYNAME_STR 		"Microsoft Corporation"

#define VER_FILEVERSION_STR \
	STR(DBG_SCRIPT_VER_MAJ) "."\
	STR(DBG_SCRIPT_VER_MIN) "."\
	STR(DBG_SCRIPT_VER_BETA)
	
#define VER_PRODUCTVERSION_STR		VER_FILEVERSION_STR
#define VER_PRODUCTNAME_STR			VER_FILEDESCRIPTION_STR
#define VER_LEGALCOPYRIGHT_STR		"(c) Microsoft Corporation"
