#pragma once

#include "../common.h"
#include <python.h>

struct ThreadObj;

_Check_return_ bool
InitThreadType();

_Check_return_ PyObject*
AllocThreadObj(
	_In_ ULONG engineId,
	_In_ ULONG threadId);

class CAutoSwitchThread
{
public:
	CAutoSwitchThread(
		_In_ const ThreadObj* thd);
	~CAutoSwitchThread();
private:
	ULONG m_PrevThreadId;
	bool m_DidSwitch;
};
