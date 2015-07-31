#include "process.h"
#include "thread.h"
#include "typedobject.h"
#include "util.h"

struct ProcessObj
{
	PyObject_HEAD
};

static PyTypeObject ProcessType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.Process",     /* tp_name */
	sizeof(ProcessObj)       /* tp_basicsize */
};

static PyObject*
Process_create_typed_object(
	_In_ PyObject* self,
	_In_ PyObject* args)
{
	CHECK_ABORT;

	PyObject *ret = nullptr;
	const char *typeName = nullptr;
	UINT64 addr = 0;
	ULONG typeId = 0;
	UINT64 modBase = 0;

	if (!PyArg_ParseTuple(args, "sK:create_typed_object", &typeName, &addr))
	{
		return nullptr;
	}

	// Lookup typeid/moduleBase from type name.
	//
	HRESULT hr = GetDllGlobals()->DebugSymbols->GetSymbolTypeId(typeName, &typeId, &modBase);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type id for type '%s'. Error 0x%08x.", typeName, hr);
		goto exit;
	}

	ret = AllocTypedObject(0, nullptr, typeName, typeId, modBase, addr, (ProcessObj*)self);
exit:
	return ret;
}

static PyObject*
Process_resolve_enum(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	CHECK_ABORT;

	PyObject *ret = nullptr;
	const char* enumTypeName = nullptr;
	UINT64 value = 0;
	UINT64 modBase = 0;
	ULONG typeId = 0;
	char enumElementName[MAX_SYMBOL_NAME_LEN] = {};
	if (!PyArg_ParseTuple(args, "sK:resolve_enum", &enumTypeName, &value))
	{
		return nullptr;
	}

	HRESULT hr = GetDllGlobals()->DebugSymbols->GetSymbolTypeId(enumTypeName, &typeId, &modBase);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type id for symbol '%s'. Error 0x%08x.", enumTypeName, hr);
		goto exit;
	}

	hr = GetDllGlobals()->DebugSymbols->GetConstantName(modBase, typeId, value, STRING_AND_CCH(enumElementName), nullptr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get element name for enum '%s' with value '%llu'. Error 0x%08x.", enumTypeName, value, hr);
		goto exit;
	}

	ret = PyUnicode_FromString(enumElementName);
exit:
	return ret;
}

static PyObject*
Process_get_global(
	_In_ PyObject* self,
	_In_ PyObject* args)
{
	CHECK_ABORT;

	PyObject *ret = nullptr;
	const char* symbol = nullptr;
	UINT64 addr = 0;
	ULONG typeId = 0;
	UINT64 modBase = 0;
	char typeName[MAX_SYMBOL_NAME_LEN] = {};
	if (!PyArg_ParseTuple(args, "s:get_global", &symbol))
	{
		return nullptr;
	}
	HRESULT hr = GetDllGlobals()->DebugSymbols->GetOffsetByName(symbol, &addr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get virtual address for symbol '%s'. Error 0x%08x.", symbol, hr);
		goto exit;
	}

	hr = GetDllGlobals()->DebugSymbols->GetSymbolTypeId(symbol, &typeId, &modBase);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type id for symbol '%s'. Error 0x%08x.", symbol, hr);
		goto exit;
	}

	hr = GetDllGlobals()->DebugSymbols->GetTypeName(modBase, typeId, STRING_AND_CCH(typeName), nullptr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type name for symbol '%s'. Error 0x%08x.", symbol, hr);
		goto exit;
	}

	ret = AllocTypedObject(0, symbol, typeName, typeId, modBase, addr, (ProcessObj*)self);
exit:
	return ret;
}

static PyObject*
Process_read_ptr(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	CHECK_ABORT;

	PyObject *ret = nullptr;
	UINT64 addr = 0;
	UINT64 ptrVal = 0;
	if (!PyArg_ParseTuple(args, "K:read_ptr", &addr))
	{
		return nullptr;
	}

	HRESULT hr = UtilReadPointer(addr, &ptrVal);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to read pointer value from address '%p'. Error 0x%08x.", addr, hr);
		goto exit;
	}

	ret = PyLong_FromUnsignedLongLong(ptrVal);

exit:
	return ret;
}

static PyObject*
Process_get_threads(
	_In_ PyObject* self,
	_In_ PyObject* /* args */)
{
	CHECK_ABORT;

	// TODO: The bulk of this code is not Python-specific. Factor it out when
	// implementing Ruby provider.
	//
	ULONG* engineThreadIds = nullptr;
	ULONG* sysThreadIds = nullptr;
	PyObject* tuple = nullptr;
	IDebugSystemObjects* sysObj = GetDllGlobals()->DebugSysObj;
	ProcessObj* proc = (ProcessObj*)self;

	ULONG cThreads = 0;
	HRESULT hr = sysObj->GetNumberThreads(&cThreads);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get number of threads. Error 0x%08x.", hr);
		goto exit;
	}

	// Get list of thread IDs.
	//
	engineThreadIds = new ULONG[cThreads];
	sysThreadIds = new ULONG[cThreads];

	hr = sysObj->GetThreadIdsByIndex(0, cThreads, engineThreadIds, sysThreadIds);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get thread IDs. Error 0x%08x.", hr);
		goto exit;
	}

	tuple = PyTuple_New(cThreads);

	// Build a tuple of Thread objects.
	//
	for (ULONG i = 0; i < cThreads; ++i)
	{
		PyObject* thd = AllocThreadObj(engineThreadIds[i], sysThreadIds[i], proc);
		if (!thd)
		{
			// Exception has already been setup by callee.
			//
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		if (PyTuple_SetItem(tuple, i, thd) != 0)
		{
			// Failed to set the item. Do not decref it. PyTuple_SetItem does
			// it internally.
			//
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}


exit:
	// Deleting nullptr is safe.
	//
	delete[] engineThreadIds;
	delete[] sysThreadIds;

	if (FAILED(hr))
	{
		// Release the tuple. This will release any held object inside the tuple.
		//
		Py_XDECREF(tuple);
		tuple = nullptr;
	}
	return tuple;
}

static PyMethodDef Process_MethodDef[] = 
{
	{
		"get_threads",
		Process_get_threads,
		METH_NOARGS,
		PyDoc_STR("Return a tuple of threads in the process")
	},
	{
		"create_typed_object",
		Process_create_typed_object,
		METH_VARARGS,
		PyDoc_STR("Return a TypedObject with a given type and address.")
	},
	{
		"read_ptr",
		Process_read_ptr,
		METH_VARARGS,
		PyDoc_STR("Read a pointer at given address.")
	},
	{
		"get_global",
		Process_get_global,
		METH_VARARGS,
		PyDoc_STR("Get a global variable as a TypedObject.")
	},
	{
		"resolve_enum",
		Process_resolve_enum,
		METH_VARARGS,
		PyDoc_STR("Get an enum element's name based on its type and value.")
	},
	{ NULL }  /* Sentinel */
};

static void
Process_dealloc(
	_In_ PyObject* self)
{
	Py_TYPE(self)->tp_free(self);
}

static PyObject*
Process_get_current_thread(
	_In_ PyObject* self,
	_In_opt_ void* /* closure */)
{
	CHECK_ABORT;

	PyObject* ret = nullptr;
	ProcessObj* proc = (ProcessObj*)self;

	// Get TEB from debug client.
	//
	ULONG engineThreadId = 0;
	ULONG systemThreadId = 0;
	HRESULT hr = GetDllGlobals()->DebugSysObj->GetCurrentThreadId(&engineThreadId);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get engine thread id. Error 0x%08x.", hr);
		goto exit;
	}

	hr = GetDllGlobals()->DebugSysObj->GetCurrentThreadSystemId(&systemThreadId);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get system thread id. Error 0x%08x.", hr);
		goto exit;
	}

	ret = AllocThreadObj(engineThreadId, systemThreadId, proc);

exit:
	return ret;
}

static PyGetSetDef Process_GetSetDef[] =
{
	{
		"current_thread",
		Process_get_current_thread,
		SetReadOnlyProperty,  // Attribute is read-only.
		PyDoc_STR("Current Thread object."),
		NULL
	},
	{ NULL }  /* Sentinel */
};

_Check_return_ bool
InitProcessType()
{
	ProcessType.tp_flags = Py_TPFLAGS_DEFAULT;
	ProcessType.tp_doc = PyDoc_STR("dbgscript.Process objects");
	ProcessType.tp_new = PyType_GenericNew;
	ProcessType.tp_dealloc = Process_dealloc;
	ProcessType.tp_methods = Process_MethodDef;
	ProcessType.tp_getset = Process_GetSetDef;

	// Finalize the type definition.
	//
	if (PyType_Ready(&ProcessType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocProcessObj()
{
	PyObject* obj = nullptr;
	PyObject* ret = nullptr;

	// Alloc a single instance of the DbgScriptOutType object. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = ProcessType.tp_new(&ProcessType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	// Do anything that can fail here.
	//

	// Transfer ownership.
	//
	ret = obj;
	obj = nullptr;

//exit:

	if (obj)
	{
		// Destructor for ProcessObj automatically frees the module map.
		//
		Py_DECREF(obj);
		obj = nullptr;
	}

	return ret;
}
