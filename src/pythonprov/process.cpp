#include "process.h"
#include "thread.h"
#include <map>

typedef std::map<UINT64, const char*> ModuleMapT;

struct ProcessObj
{
	PyObject_HEAD
	ModuleMapT ModuleMap;
};

static PyTypeObject ProcessType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.Process",     /* tp_name */
	sizeof(ProcessObj)           /* tp_basicsize */
};

static PyObject* g_Process;  // Single process for now.

static PyObject*
Process_get_threads(
	_In_ PyObject* self,
	_In_ PyObject* /* args */)
{
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
	{ NULL }  /* Sentinel */
};

static void
freeModuleMap(
	_In_ ModuleMapT* map)
{
	for (ModuleMapT::const_iterator it = map->begin();
	it != map->end();
		++it)
	{
		delete[] it->second;
	}
	map->~ModuleMapT();
}

static void
Process_dealloc(
	_In_ PyObject* self)
{
	ProcessObj* proc = (ProcessObj*)self;

	// Free the module map.
	//
	freeModuleMap(&proc->ModuleMap);
	Py_TYPE(self)->tp_free(self);
}

static PyObject*
Process_get_current_thread(
	_In_ PyObject* self,
	_In_opt_ void* /* closure */)
{
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

// Attribute is read-only.
//
static int
Process_set_current_thread(PyObject* /* self */, PyObject* /* value */, void* /* closure */)
{
	PyErr_SetString(PyExc_AttributeError, "readonly attribute");
	return -1;
}

static PyGetSetDef Process_GetSetDef[] =
{
	{
		"current_thread",
		Process_get_current_thread,
		Process_set_current_thread,
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
	HRESULT hr = S_OK;
	PyObject* obj = nullptr;
	PyObject* ret = nullptr;
	DEBUG_MODULE_PARAMETERS* modParams = nullptr;
	IDebugSymbols3* dbgSym = GetDllGlobals()->DebugSymbols;

	// Alloc a single instance of the DbgScriptOutType object. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = ProcessType.tp_new(&ProcessType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	ProcessObj* proc = (ProcessObj*)obj;
	new (&proc->ModuleMap) ModuleMapT;

	ULONG cLoadedModules = 0;
	ULONG cUnloadedModules = 0;
	hr = dbgSym->GetNumberModules(&cLoadedModules, &cUnloadedModules);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get number of modules. Error 0x%08x.", hr);
		goto exit;
	}

	modParams = new DEBUG_MODULE_PARAMETERS[cLoadedModules];
	for (ULONG i = 0; i < cLoadedModules; ++i)
	{
		ULONG nameSize = 0;
		UINT64 modBase = 0;
		hr = dbgSym->GetModuleNameString(DEBUG_MODNAME_MODULE, i, 0, nullptr, 0, &nameSize);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to get size of module name. Error 0x%08x.", hr);
			goto exit;
		}

		char* name = new char[nameSize];
		hr = dbgSym->GetModuleNameString(DEBUG_MODNAME_MODULE, i, 0, name, nameSize, nullptr);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to get module name. Error 0x%08x.", hr);
			goto exit;
		}
		hr = dbgSym->GetModuleByIndex(i, &modBase);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "Failed to get module base. Error 0x%08x.", hr);
			goto exit;
		}

		// Insert into map.
		//
		proc->ModuleMap[modBase] = name;
	}

	// Transfer ownership.
	//
	ret = obj;
	obj = nullptr;

exit:

	if (obj)
	{
		// Destructor for ProcessObj automatically frees the module map.
		//
		Py_DECREF(obj);
		obj = nullptr;
	}

	if (modParams)
	{
		delete[] modParams;
		modParams = nullptr;
	}

	return ret;
}

_Check_return_ const char*
ProcessObjGetModuleName(
	_In_ ProcessObj* proc,
	_In_ UINT64 modBase)
{
	return proc->ModuleMap[modBase];
}