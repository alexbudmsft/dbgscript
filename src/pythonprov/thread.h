#pragma once

#include "../common.h"
#include <python.h>

struct ThreadObj;
struct ProcessObj;

struct ThreadObj
{
	PyObject_HEAD

	DbgScriptThread Thread;
};

_Check_return_ bool
InitThreadType();

_Check_return_ PyObject*
AllocThreadObj(
	_In_ ULONG engineId,
	_In_ ULONG threadId);