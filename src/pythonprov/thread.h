#pragma once

#include "../common.h"
#include <python.h>

struct ThreadObj;
struct ProcessObj;

struct ThreadObj
{
	PyObject_HEAD

	DbgScriptThread Thread;

	// Parent process object.
	//
	ProcessObj* Process;
};

_Check_return_ bool
InitThreadType();

_Check_return_ PyObject*
AllocThreadObj(
	_In_ ULONG engineId,
	_In_ ULONG threadId,
	_In_ ProcessObj* proc);

_Check_return_ ProcessObj*
ThreadObjGetProcess(
	_In_ const ThreadObj* thd);