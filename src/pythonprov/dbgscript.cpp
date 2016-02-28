//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dbgscript.cpp
// @Author: alexbud
//
// Purpose:
//
//  DbgScript module for Python Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "dbgscript.h"
#include "util.h"
#include "../support/symcache.h"
#include "common.h"

// Python classes.
//
#include "dbgscriptio.h"
#include "process.h"
#include "thread.h"
#include "stackframe.h"
#include "typedobject.h"

//------------------------------------------------------------------------------
// Function: createTypedObjectHelper
//
// Description:
//
//  Helper to create a typed object or pointer.
//
static PyObject*
createTypedObjectHelper(
	_In_ PyObject* args,
	_In_ bool wantPointer)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject *ret = nullptr;
	const char *typeName = nullptr;
	UINT64 addr = 0;

	if (!PyArg_ParseTuple(args, "sK", &typeName, &addr))
	{
		return nullptr;
	}

	// Lookup typeid/moduleBase from type name.
	//
	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, typeName);
	if (!typeInfo)
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type id for type '%s'.", typeName);
		goto exit;
	}

	ret = AllocTypedObject(
		0, nullptr, typeInfo->TypeId, typeInfo->ModuleBase, addr, wantPointer);
exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_create_typed_object
//
// Synopsis:
// 
//  dbgscript.create_typed_object(type, addr) -> TypedObject
//
// Description:
//
//  Create a typed object from a given type and address.
//
static PyObject*
dbgscript_create_typed_object(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	return createTypedObjectHelper(args, false /* wantPtr */);
}

//------------------------------------------------------------------------------
// Function: dbgscript_create_typed_pointer
//
// Synopsis:
// 
//  dbgscript.create_typed_pointer(type, addr) -> TypedObject
//
// Description:
//
//  Create a typed pointer from a given type and address. 'type' is the base
//  type from which to create a pointer. E.g. for int*, the base type is int.
//
static PyObject*
dbgscript_create_typed_pointer(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	return createTypedObjectHelper(args, true /* wantPtr */);
}

//------------------------------------------------------------------------------
// Function: dbgscript_resolve_enum
//
// Synopsis:
// 
//  dbgscript.resolve_enum(enum, val) -> str
//
// Description:
//
//  Derive the textual enumerant string from an enum type and value.
//
static PyObject*
dbgscript_resolve_enum(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject *ret = nullptr;
	const char* enumTypeName = nullptr;
	UINT64 value = 0;
	char enumElementName[MAX_SYMBOL_NAME_LEN] = {};
	if (!PyArg_ParseTuple(args, "sK:resolve_enum", &enumTypeName, &value))
	{
		return nullptr;
	}

	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, enumTypeName);
	if (!typeInfo)
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type id for type '%s'.", enumTypeName);
		goto exit;
	}

	HRESULT hr = hostCtxt->DebugSymbols->GetConstantName(
		typeInfo->ModuleBase,
		typeInfo->TypeId,
		value,
		STRING_AND_CCH(enumElementName),
		nullptr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get element name for enum '%s' with value '%llu'. Error 0x%08x.", enumTypeName, value, hr);
		goto exit;
	}

	ret = PyUnicode_FromString(enumElementName);
exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_get_global
//
// Synopsis:
// 
//  dbgscript.get_global(sym) -> TypedObject
//
// Description:
//
//  Create a typed object from a global variable identified by 'sym'.
//
static PyObject*
dbgscript_get_global(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject *ret = nullptr;
	const char* symbol = nullptr;
	UINT64 addr = 0;
	if (!PyArg_ParseTuple(args, "s:get_global", &symbol))
	{
		return nullptr;
	}
	HRESULT hr = hostCtxt->DebugSymbols->GetOffsetByName(symbol, &addr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to get virtual address for symbol '%s'. Error 0x%08x.", symbol, hr);
		goto exit;
	}

	ModuleAndTypeId* typeInfo = GetCachedSymbolType(
		hostCtxt, symbol);
	if (!typeInfo)
	{
		PyErr_Format(PyExc_ValueError, "Failed to get type id for type '%s'.", symbol);
		goto exit;
	}

	ret = AllocTypedObject(
		0,
		symbol,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		addr,
		false /* wantPointer */);
exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_get_peb
//
// Synopsis:
// 
//  dbgscript.get_peb() -> int
//
// Description:
//
//  Get the address of the current process' PEB.
//
static PyObject*
dbgscript_get_peb(
	_In_ PyObject* /*self*/,
	_In_ PyObject* /*args*/)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	PyObject *ret = nullptr;

	UINT64 addr = 0;
	HRESULT hr = UtilGetPeb(hostCtxt, &addr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_RuntimeError, "Failed to get PEB. Error 0x%08x.", hr);
		goto exit;
	}
	
	ret = PyLong_FromUnsignedLongLong(addr);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_read_ptr
//
// Synopsis:
// 
//  dbgscript.read_ptr(addr) -> int
//
// Description:
//
//  Read a pointer value at a given address.
//
static PyObject*
dbgscript_read_ptr(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject *ret = nullptr;
	UINT64 addr = 0;
	UINT64 ptrVal = 0;
	if (!PyArg_ParseTuple(args, "K:read_ptr", &addr))
	{
		return nullptr;
	}

	HRESULT hr = UtilReadPointer(hostCtxt, addr, &ptrVal);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to read pointer value from address '%p'. Error 0x%08x.", addr, hr);
		goto exit;
	}

	ret = PyLong_FromUnsignedLongLong(ptrVal);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_field_offset
//
// Synopsis:
// 
//  dbgscript.field_offset(type, field) -> int
//
// Description:
//
//  Return the offset of 'field' in 'type'.
//
static PyObject*
dbgscript_field_offset(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject *ret = nullptr;
	const char* type = nullptr;
	const char* field = nullptr;
	ULONG offset = 0;
	if (!PyArg_ParseTuple(args, "ss:field_offset", &type, &field))
	{
		return nullptr;
	}

	HRESULT hr = UtilGetFieldOffset(hostCtxt, type, field, &offset);
	if (FAILED(hr))
	{
		PyErr_Format(
			PyExc_ValueError,
			"Failed to get field offset for type '%s' and field '%s'. Error 0x%08x.",
			type,
			field,
			hr);
		goto exit;
	}

	ret = PyLong_FromUnsignedLong(offset);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_get_type_size
//
// Synopsis:
// 
//  dbgscript.get_type_size(type) -> int
//
// Description:
//
//  Return the size of 'type' in bytes.
//
static PyObject*
dbgscript_get_type_size(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject* ret = nullptr;
	const char* type = nullptr;
	ULONG size = 0;
	if (!PyArg_ParseTuple(args, "s:get_type_size", &type))
	{
		return nullptr;
	}

	HRESULT hr = UtilGetTypeSize(hostCtxt, type, &size);
	if (FAILED(hr))
	{
		PyErr_Format(
			PyExc_ValueError,
			"Failed to get size of type '%s'. Error 0x%08x.",
			type,
			hr);
		goto exit;
	}

	ret = PyLong_FromUnsignedLong(size);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_read_bytes
//
// Synopsis:
// 
//  dbgscript.read_bytes(addr, count) -> bytes
//
// Description:
//
//  Read 'count' bytes from 'addr'.
//
static PyObject*
dbgscript_read_bytes(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	PyObject *ret = nullptr;

	UINT64 addr = 0;
	ULONG count = 0;
	if (!PyArg_ParseTuple(args, "Kk:read_bytes", &addr, &count))
	{
		goto exit;
	}

	ret = PyReadBytes(addr, count);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_read_string
//
// Synopsis:
// 
//  dbgscript.read_string(addr [, count]) -> str
//
// Description:
//
//  Read a string from 'addr' (up to 'count' bytes).
//
static PyObject*
dbgscript_read_string(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	PyObject *ret = nullptr;

	UINT64 addr = 0;
	
	// Initialize to default value. -1 means read up to NUL.
	//
	int count = -1;
	
	if (!PyArg_ParseTuple(args, "K|i:read_string", &addr, &count))
	{
		goto exit;
	}

	ret = PyReadString(addr, count);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_read_wide_string
//
// Synopsis:
// 
//  dbgscript.read_wide_string(addr [, count]) -> str
//
// Description:
//
//  Read a string from 'addr' (up to 'count' bytes).
//
static PyObject*
dbgscript_read_wide_string(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	PyObject *ret = nullptr;

	UINT64 addr = 0;
	
	// Initialize to default value. -1 means read up to NUL.
	//
	int count = -1;
	
	if (!PyArg_ParseTuple(args, "K|i:read_wide_string", &addr, &count))
	{
		goto exit;
	}

	ret = PyReadWideString(addr, count);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_get_nearest_sym
//
// Synopsis:
// 
//  dbgscript.get_nearest_sym(addr) -> int
//
// Description:
//
//  Lookup nearest symbol name at given address.
//
static PyObject*
dbgscript_get_nearest_sym(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject *ret = nullptr;
	UINT64 addr = 0;
	char name[MAX_SYMBOL_NAME_LEN] = {};
	if (!PyArg_ParseTuple(args, "K:get_nearest_sym", &addr))
	{
		return nullptr;
	}

	HRESULT hr = UtilGetNearestSymbol(hostCtxt, addr, name);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_ValueError, "Failed to resolve symbol from address %p. Error 0x%08x.", addr, hr);
		goto exit;
	}

	ret = PyUnicode_FromString(name);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_get_threads
//
// Synopsis:
// 
//  dbgscript.get_threads() -> tuple of Thread
//
// Description:
//
//  Get a collection of threads in the process.
//
static PyObject*
dbgscript_get_threads(
	_In_ PyObject* /*self*/,
	_In_ PyObject* /* args */)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	ULONG cThreads = 0;
	ULONG* engineThreadIds = nullptr;
	ULONG* sysThreadIds = nullptr;
	PyObject* tuple = nullptr;
	HRESULT hr = UtilCountThreads(hostCtxt, &cThreads);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "UtilCountThreads failed. Error 0x%08x.", hr);
		goto exit;
	}

	// Get list of thread IDs.
	//
	engineThreadIds = new ULONG[cThreads];
	sysThreadIds = new ULONG[cThreads];

	hr = UtilEnumThreads(hostCtxt, cThreads, engineThreadIds, sysThreadIds);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "UtilEnumThreads failed. Error 0x%08x.", hr);
		goto exit;
	}

	tuple = PyTuple_New(cThreads);

	// Build a tuple of Thread objects.
	//
	for (ULONG i = 0; i < cThreads; ++i)
	{
		PyObject* thd = AllocThreadObj(engineThreadIds[i], sysThreadIds[i]);
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

//------------------------------------------------------------------------------
// Function: dbgscript_get_current_thread
//
// Synopsis:
// 
//  dbgscript.get_current_thread() -> Thread
//
// Description:
//
//  Get current thread in process.
//
static PyObject*
dbgscript_get_current_thread(
	_In_ PyObject* /*self*/,
	_In_ PyObject* /*args*/)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	PyObject* ret = nullptr;

	// Get TEB from debug client.
	//
	ULONG engineThreadId = 0;
	ULONG systemThreadId = 0;
	HRESULT hr = hostCtxt->DebugSysObj->GetCurrentThreadId(&engineThreadId);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get engine thread id. Error 0x%08x.", hr);
		goto exit;
	}

	hr = hostCtxt->DebugSysObj->GetCurrentThreadSystemId(&systemThreadId);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get system thread id. Error 0x%08x.", hr);
		goto exit;
	}

	ret = AllocThreadObj(engineThreadId, systemThreadId);

exit:
	return ret;
}

//------------------------------------------------------------------------------
// Function: dbgscript_start_buffering
//
// Synopsis:
// 
//  dbgscript.start_buffering() -> None
//
// Description:
//
//  Start an output buffering session.
//
static PyObject*
dbgscript_start_buffering(
	_In_ PyObject* /*self*/,
	_In_ PyObject* /*args*/)
{
	// Currently this is single-threaded access, but will make it simpler in
	// case we ever have more than one concurrent client.
	//
	InterlockedIncrement(&GetPythonProvGlobals()->HostCtxt->IsBuffering);

	Py_RETURN_NONE;
}

//------------------------------------------------------------------------------
// Function: dbgscript_stop_buffering
//
// Synopsis:
// 
//  dbgscript.stop_buffering() -> None
//
// Description:
//
//  Stop an output buffering session.
//
static PyObject*
dbgscript_stop_buffering(
	_In_ PyObject* /*self*/,
	_In_ PyObject* /*args*/)
{
	// Currently this is single-threaded access, but will make it simpler in
	// case we ever have more than one concurrent client.
	//
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	const LONG newVal = InterlockedDecrement(&hostCtxt->IsBuffering);
	if (newVal < 0)
	{
		GetPythonProvGlobals()->HostCtxt->IsBuffering = 0;
		PyErr_SetString(PyExc_RuntimeError, "Can't stop buffering if it isn't started.");
		return nullptr;
	}

	// If the buffer refcount hit zero, flush remaining buffered content, if any.
	//
	if (newVal == 0)
	{
		UtilFlushMessageBuffer(hostCtxt);
	}

	Py_RETURN_NONE;
}

//------------------------------------------------------------------------------
// Function: dbgscript_execute_command
//
// Synopsis:
// 
//  dbgscript.execute_command(cmd) -> None
//
// Description:
//
//  Execute a debugger command and output results.
//
static PyObject*
dbgscript_execute_command(
	_In_ PyObject* /*self*/,
	_In_ PyObject* args)
{
	const char* command = nullptr;
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	CHECK_ABORT(hostCtxt);
	
	if (!PyArg_ParseTuple(args, "s:execute_command", &command))
	{
		return nullptr;
	}

	hr = UtilExecuteCommand(hostCtxt, command);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "UtilExecuteCommand failed. Error 0x%08x.", hr);
		goto exit;
	}

exit:
	Py_RETURN_NONE;
}

static PyMethodDef dbgscript_MethodsDef[] = 
{
	{
		"execute_command",
		dbgscript_execute_command,
		METH_VARARGS,
		PyDoc_STR("Execute a debugger command.")
	},
	{
		"start_buffering",
		dbgscript_start_buffering,
		METH_NOARGS,
		PyDoc_STR("Start buffering output.")
	},
	{
		"stop_buffering",
		dbgscript_stop_buffering,
		METH_NOARGS,
		PyDoc_STR("Stop buffering output.")
	},
	{
		"get_threads",
		dbgscript_get_threads,
		METH_NOARGS,
		PyDoc_STR("Return a tuple of threads in the process")
	},
	{
		"current_thread",
		dbgscript_get_current_thread,
		METH_NOARGS,
		PyDoc_STR("Return the current Thread."),
	},
	{
		"create_typed_object",
		dbgscript_create_typed_object,
		METH_VARARGS,
		PyDoc_STR("Return a TypedObject with a given type and address.")
	},
	{
		"create_typed_pointer",
		dbgscript_create_typed_pointer,
		METH_VARARGS,
		PyDoc_STR("Return a pointer to a TypedObject with a given type and address.")
	},
	{
		"read_ptr",
		dbgscript_read_ptr,
		METH_VARARGS,
		PyDoc_STR("Read a pointer at given address.")
	},
	{
		"read_bytes",
		dbgscript_read_bytes,
		METH_VARARGS,
		PyDoc_STR("Read bytes from given address.")
	},
	{
		"read_string",
		dbgscript_read_string,
		METH_VARARGS,
		PyDoc_STR("Read a string from given address.")
	},
	{
		"read_wide_string",
		dbgscript_read_wide_string,
		METH_VARARGS,
		PyDoc_STR("Read a wide string from given address.")
	},
	{
		"field_offset",
		dbgscript_field_offset,
		METH_VARARGS,
		PyDoc_STR("Get field offset given a type and address.")
	},
	{
		"get_type_size",
		dbgscript_get_type_size,
		METH_VARARGS,
		PyDoc_STR("Get size of given a type in bytes.")
	},
	{
		"get_nearest_sym",
		dbgscript_get_nearest_sym,
		METH_VARARGS,
		PyDoc_STR("Lookup the nearest symbol at given address.")
	},
	{
		"get_global",
		dbgscript_get_global,
		METH_VARARGS,
		PyDoc_STR("Get a global variable as a TypedObject.")
	},
	{
		"get_peb",
		dbgscript_get_peb,
		METH_NOARGS,
		PyDoc_STR("Get the current PEB address.")
	},
	{
		"resolve_enum",
		dbgscript_resolve_enum,
		METH_VARARGS,
		PyDoc_STR("Get an enum element's name based on its type and value.")
	},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

// DbgScript Module Definition.
//
PyModuleDef dbgscript_ModuleDef =
{
	PyModuleDef_HEAD_INIT,
	x_DbgScriptModuleName,  // Name
	PyDoc_STR("dbgscript Module"),       // Doc
	-1,  // size
	dbgscript_MethodsDef,  // Methods
};

_Check_return_ bool
InitTypes()
{
	// TODO: Make this a table of callbacks.
	//
	if (!InitDbgScriptIOType())
	{
		return false;
	}

	if (!InitThreadType())
	{
		return false;
	}

	if (!InitStackFrameType())
	{
		return false;
	}

	if (!InitTypedObjectType())
	{
		return false;
	}
	return true;
}

static PyObject* g_DbgScriptIO;

// Module initialization function for 'dbgscript'.
//
PyMODINIT_FUNC
PyInit_dbgscript()
{
	if (!InitTypes())
	{
		return nullptr;
	}

	g_DbgScriptIO = AllocDbgScriptIOObj();
	if (!g_DbgScriptIO)
	{
		return nullptr;
	}

	RedirectStdIO(g_DbgScriptIO);

	// Create a module object.
	//
	PyObject* module = PyModule_Create(&dbgscript_ModuleDef);
	if (!module)
	{
		return nullptr;
	}

	return module;
}


