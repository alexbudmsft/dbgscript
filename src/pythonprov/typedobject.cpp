#include <windows.h>

#include "typedobject.h"
#include <strsafe.h>
#include <structmember.h>
#include "process.h"
#include "util.h"
#include "common.h"

struct ProcessObj;

struct TypedObject
{
	PyObject_HEAD

	// Backpointer to process object.
	//
	ProcessObj* Process;

	DbgScriptTypedObject Data;
};

static PyMemberDef TypedObject_MemberDef[] =
{
	{ "size", T_ULONG, offsetof(TypedObject, Data.TypedData.Size), READONLY },
	{ "name", T_STRING_INPLACE, offsetof(TypedObject, Data.Name), READONLY },
	{ "type", T_STRING_INPLACE, offsetof(TypedObject, Data.TypeName), READONLY },
	{ "address", T_ULONGLONG, offsetof(TypedObject, Data.TypedData.Offset), READONLY },
	{ NULL }
};

static PyTypeObject TypedObjectType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.TypedObject",     /* tp_name */
	sizeof(TypedObject)       /* tp_basicsize */
};

// Call when you already have a DEBUG_TYPED_DATA you want wrapped in a TypedObject.
//
static _Check_return_ PyObject*
allocSubTypedObject(
	_In_z_ const char* name,
	_In_ const DEBUG_TYPED_DATA* typedData,
	_In_ ProcessObj* proc)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	PyObject* obj = nullptr;
	PyObject* ret = nullptr;

	// Alloc a single instance of the TypedObjectType class. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = TypedObjectType.tp_new(&TypedObjectType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	// Set up fields.
	//
	TypedObject* typObj = (TypedObject*)obj;

	Py_INCREF(proc);
	typObj->Process = proc;

	HRESULT hr = DsWrapTypedData(hostCtxt, name, typedData, &typObj->Data);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsWrapTypedData failed. Error 0x%08x.", hr);
		goto exit;
	}
	
	// Transfer ownership on success.
	//
	ret = obj;
	obj = nullptr;

exit:

	if (obj)
	{
		Py_DECREF(obj);
		obj = nullptr;
	}
	return ret;
}

static bool checkTypedData(
	_In_ TypedObject* typedObj)
{
	if (!typedObj->Data.TypedDataValid)
	{
		// This object has no typed data. It must have been a null ptr.
		//
		PyErr_SetString(PyExc_AttributeError, "Object has no typed data. Can't get fields.");
		return false;
	}
	return true;
}

// Get an array element. I.e. object[i], where i is int in Python.
//
static PyObject*
TypedObject_sequence_get_item(
	_In_ PyObject* self,
	_In_ Py_ssize_t index)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	TypedObject* typObj = (TypedObject*)self;
	PyObject* ret = nullptr;

	if (!checkTypedData(typObj))
	{
		return nullptr;
	}

	if (typObj->Data.TypedData.Tag != SymTagPointerType &&
		typObj->Data.TypedData.Tag != SymTagArrayType)
	{
		// Not a pointer or array.
		//
		// This object has no typed data. It must have been a null ptr.
		//
		PyErr_SetString(PyExc_AttributeError, "Object not a pointer or array.");
		return nullptr;
	}

	EXT_TYPED_DATA request = {};
	EXT_TYPED_DATA response = {};
	request.Operation = EXT_TDOP_GET_ARRAY_ELEMENT;
	request.InData = typObj->Data.TypedData;
	request.In64 = index;

	static_assert(sizeof(request) == sizeof(response),
		"Request and response must be equi-sized");

	HRESULT hr = hostCtxt->DebugAdvanced->Request(
		DEBUG_REQUEST_EXT_TYPED_DATA_ANSI,
		&request,
		sizeof(request),
		&response,
		sizeof(response),
		nullptr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "EXT_TDOP_GET_ARRAY_ELEMENT operation failed. Error 0x%08x.", hr);
		goto exit;
	}

	// Array elements have no name.
	//
	ret = allocSubTypedObject("<unnamed>", &response.OutData, typObj->Process);
exit:
	return ret;
}

// Get a field from a typed object, constructing a new TypedObject in the process.
//
static PyObject*
TypedObject_mapping_subscript(
	_In_ PyObject* self,
	_In_ PyObject* key)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	TypedObject* typedObj = (TypedObject*)self;
	PyObject* newTypedObj = nullptr;
	EXT_TYPED_DATA* request = nullptr;
	EXT_TYPED_DATA* responseBuf = nullptr;
	BYTE* requestBuf = nullptr;

	// A type can only be a mapping or a sequence. If the [] syntax is used,
	// python will first check for mapping support and call that API, ignoring
	// sequence APIs.
	//
	// Since we want to support both, we will check if the key is an int, and
	// if so, call our own array-style lookup routine.
	//
	if (PyLong_Check(key))
	{
		// Call int-flavoured API.
		//
		Py_ssize_t index = PyLong_AsSsize_t(key);
		if (index == -1)
		{
			return nullptr;
		}

		return TypedObject_sequence_get_item(self, index);
	}

	HRESULT hr = S_OK;
	if (!PyUnicode_Check(key))
	{
		PyErr_SetString(PyExc_KeyError, "key must be a unicode object");
		return nullptr;
	}

	if (!checkTypedData(typedObj))
	{
		return nullptr;
	}

	PyObject* asciiStrAsBytes = PyUnicode_AsASCIIString(key);
	if (!asciiStrAsBytes)
	{
		return nullptr;
	}

	const char* fieldName = PyBytes_AsString(asciiStrAsBytes);
	if (!fieldName)
	{
		return nullptr;
	}

	// TODO: Encapsulate this logic into a separate class perhaps.
	//
	const ULONG fieldNameLen = (ULONG)strlen(fieldName);
	const ULONG reqSize = sizeof(EXT_TYPED_DATA) + fieldNameLen + 1;

	requestBuf = (BYTE*)malloc(reqSize);
	if (!requestBuf)
	{
		PyErr_NoMemory();
		goto exit;
	}

	// Response buffer must be as big as request since dbgeng memcpy's from
	// request to response as a first step.
	//
	responseBuf = (EXT_TYPED_DATA*)malloc(reqSize);
	if (!responseBuf)
	{
		PyErr_NoMemory();
		goto exit;
	}

	memset(requestBuf, 0, reqSize);

	// TODO: Currently this doesn't support register-based variables, as the
	// virtual address is not correct.
	// Figure out what to do about them.
	//
	request = (EXT_TYPED_DATA*)requestBuf;
	request->Operation = EXT_TDOP_GET_FIELD;
	request->InData = typedObj->Data.TypedData;
	request->InStrIndex = sizeof(EXT_TYPED_DATA);

	// Must be NULL terminated.
	//
	memcpy(requestBuf + sizeof(EXT_TYPED_DATA), fieldName, fieldNameLen + 1);

	hr = hostCtxt->DebugAdvanced->Request(
		DEBUG_REQUEST_EXT_TYPED_DATA_ANSI,
		request,
		reqSize,
		responseBuf,
		reqSize,
		nullptr);
	if (hr == E_NOINTERFACE)
	{
		// This means there was no such member.
		//
		PyErr_SetString(PyExc_KeyError, "No such field.");
		goto exit;
	}
	else if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "EXT_TDOP_GET_FIELD operation failed. Error 0x%08x.", hr);
		goto exit;
	}

	newTypedObj = allocSubTypedObject(fieldName, &responseBuf->OutData, typedObj->Process);
	if (!newTypedObj)
	{
		goto exit;
	}

exit:
	if (requestBuf)
	{
		free(requestBuf);
		requestBuf = nullptr;
	}
	if (responseBuf)
	{
		free(responseBuf);
		responseBuf = nullptr;
	}

	return newTypedObj;
}

static void
TypedObject_dealloc(
	_In_ PyObject* self)
{
	TypedObject* typObj = (TypedObject*)self;

	// Release ref on the Process process object.
	//
	Py_DECREF(typObj->Process);

	// Free ourselves.
	//
	Py_TYPE(self)->tp_free(self);
}

static PyMappingMethods TypedObject_MappingDef =
{
	nullptr,  // mp_length
	TypedObject_mapping_subscript,  // mp_subscript
	nullptr   // mp_ass_subscript
};

static PyObject*
pyValueFromCValue(
	_In_ TypedObject* typObj)
{
	assert(typObj->Data.ValueValid);
	assert(typObj->Data.TypedDataValid);
	const TypedObjectValue* cValue = &typObj->Data.Value;
	DEBUG_TYPED_DATA* typedData = &typObj->Data.TypedData;
	PyObject* ret = nullptr;

	if (typedData->Tag == SymTagPointerType)
	{
		ret = PyLong_FromUnsignedLongLong(cValue->Value.UI64Val);
	}
	else if (typedData->Tag == SymTagEnum)
	{
		ret = PyLong_FromUnsignedLong(cValue->Value.DwVal);
	}
	else
	{
		assert(typedData->Tag == SymTagBaseType);

		switch (typedData->BaseTypeId)
		{
		case DNTYPE_CHAR:
			assert(typedData->Size == 1);
			ret = PyUnicode_FromOrdinal(cValue->Value.ByteVal);
			break;
		case DNTYPE_INT8:
		case DNTYPE_UINT8:
			assert(typedData->Size == 1);
			ret = PyLong_FromLong(cValue->Value.ByteVal);
			break;
		case DNTYPE_INT16:
		case DNTYPE_UINT16:
			assert(typedData->Size == sizeof(WORD));
			ret = PyLong_FromLong(cValue->Value.WordVal);
			break;
		case DNTYPE_WCHAR:
			static_assert(sizeof(WCHAR) == sizeof(WORD), "Assume WCHAR is 2 bytes");
			assert(typedData->Size == sizeof(WORD));
			ret = PyUnicode_FromOrdinal(cValue->Value.WordVal);
			break;
		case DNTYPE_INT32:
		case DNTYPE_LONG32:
			assert(typedData->Size == sizeof(DWORD));
			ret = PyLong_FromLong((long)cValue->Value.DwVal);
			break;
		case DNTYPE_UINT32:
		case DNTYPE_ULONG32:
			assert(typedData->Size == sizeof(DWORD));
			ret = PyLong_FromUnsignedLong(cValue->Value.DwVal);
			break;
		case DNTYPE_INT64:
			assert(typedData->Size == sizeof(INT64));
			ret = PyLong_FromLongLong(cValue->Value.I64Val);
			break;
		case DNTYPE_UINT64:
			assert(typedData->Size == sizeof(UINT64));
			ret = PyLong_FromUnsignedLongLong(cValue->Value.UI64Val);
			break;
		case DNTYPE_BOOL:
			assert(typedData->Size == sizeof(BOOL));
			ret = PyBool_FromLong(!!cValue->Value.BoolVal);
			break;
		case DNTYPE_FLOAT32:
			assert(typedData->Size == sizeof(float));
			ret = PyFloat_FromDouble(cValue->Value.FloatVal);
			break;
		case DNTYPE_FLOAT64:
			assert(typedData->Size == sizeof(double));
			ret = PyFloat_FromDouble(cValue->Value.DoubleVal);
			break;
		default:
			PyErr_Format(PyExc_AttributeError, "Unsupported type id: %d (%s)",
				typedData->BaseTypeId,
				typObj->Data.TypeName);
			break;
		}
	}
	return ret;
}

// __str__ method.
//
static PyObject*
TypedObject_str(
	_In_ PyObject* /*self*/)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);

	// String formatting is really slow.
	//
	return PyUnicode_FromString("dbgscript.TypedObject");
}

static PyObject*
TypedObject_get_value(
	_In_ PyObject* self,
	_In_opt_ void* /* closure */)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	TypedObject* typObj = (TypedObject*)self;
	PyObject* ret = nullptr;

	if (!typObj->Data.TypedDataValid)
	{
		PyErr_SetString(PyExc_AttributeError, "No typed data available.");
		return nullptr;
	}

	bool primitiveType = false;

	switch (typObj->Data.TypedData.Tag)
	{
	case SymTagBaseType:
	case SymTagPointerType:
	case SymTagEnum:
		primitiveType = true;
		break;
	}

	if (!primitiveType)
	{
		PyErr_SetString(PyExc_AttributeError, "Not a primitive type.");
		return nullptr;
	}

	// Read the appropriate size from memory.
	//
	// What primitive type is bigger than 8 bytes?
	//
	ULONG cbRead = 0;
	assert(typObj->Data.TypedData.Size <= 8);
	HRESULT hr = hostCtxt->DebugSymbols->ReadTypedDataVirtual(
		typObj->Data.TypedData.Offset,
		typObj->Data.TypedData.ModBase,
		typObj->Data.TypedData.TypeId,
		&typObj->Data.Value.Value,
		sizeof(typObj->Data.Value.Value),
		&cbRead);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to read typed data. Error 0x%08x.", hr);
		goto exit;
	}
	assert(cbRead == typObj->Data.TypedData.Size);
	
	// Value has been populated.
	//
	typObj->Data.ValueValid = true;

	ret = pyValueFromCValue(typObj);

exit:

	return ret;
}

// len(obj)
//
static Py_ssize_t
TypedObject_sequence_length(
	_In_ PyObject* self)
{
	TypedObject* typObj = (TypedObject*)self;

	if (!checkTypedData(typObj))
	{
		return -1;
	}

	if (typObj->Data.TypedData.Tag != SymTagPointerType &&
		typObj->Data.TypedData.Tag != SymTagArrayType)
	{
		// Not a pointer or array.
		//
		// This object has no typed data. It must have been a null ptr.
		//
		PyErr_SetString(PyExc_AttributeError, "Object not a pointer or array.");
		return -1;
	}

	// Get the zero'th item in order to get its size.
	//
	TypedObject* tmp = (TypedObject*)TypedObject_sequence_get_item(self, 0);
	if (!tmp)
	{
		return -1;
	}

	assert(tmp->Data.TypedDataValid);
	const ULONG elemSize = tmp->Data.TypedData.Size;

	// Release the temporary object.
	//
	Py_DECREF(tmp);

	return typObj->Data.TypedData.Size / elemSize;
}

static PyGetSetDef TypedObject_GetSetDef[] =
{
	{
		"value",
		TypedObject_get_value,
		SetReadOnlyProperty,
		PyDoc_STR("Value of field if primitive type."),
		NULL
	},
	{ NULL }  /* Sentinel */
};

_Check_return_ bool
InitTypedObjectType()
{
	static PySequenceMethods s_SequenceMethodsDef;
	s_SequenceMethodsDef.sq_item = TypedObject_sequence_get_item;
	s_SequenceMethodsDef.sq_length = TypedObject_sequence_length;

	TypedObjectType.tp_flags = Py_TPFLAGS_DEFAULT;
	TypedObjectType.tp_doc = PyDoc_STR("dbgscript.typObjbol objects");
	TypedObjectType.tp_members = TypedObject_MemberDef;
	TypedObjectType.tp_getset = TypedObject_GetSetDef;
	TypedObjectType.tp_new = PyType_GenericNew;
	TypedObjectType.tp_str = TypedObject_str;
	TypedObjectType.tp_dealloc = TypedObject_dealloc;
	TypedObjectType.tp_as_mapping = &TypedObject_MappingDef;
	TypedObjectType.tp_as_sequence = &s_SequenceMethodsDef;

	// Finalize the type definition.
	//
	if (PyType_Ready(&TypedObjectType) < 0)
	{
		return false;
	}
	return true;
}

// Call when you want to create a brand-new TypedObject from scratch. I.e.
// you don't have a DEBUG_TYPED_DATA to begin with.
//
// E.g. synthesizing a TypedObject from an address and type.
//
_Check_return_ PyObject*
AllocTypedObject(
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_z_ const char* type,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_In_ ProcessObj* proc)
{
	PyObject* obj = nullptr;
	PyObject* ret = nullptr;
	HRESULT hr = S_OK;

	// Alloc a single instance of the TypedObjectType class. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = TypedObjectType.tp_new(&TypedObjectType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	// Set up fields.
	//
	TypedObject* typObj = (TypedObject*)obj;

	Py_INCREF(proc);
	typObj->Process = proc;
	
	hr = DsInitializeTypedObject(
		GetPythonProvGlobals()->HostCtxt,
		size,
		name,
		type,
		typeId,
		moduleBase,
		virtualAddress,
		&typObj->Data);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsInitializeTypedObject failed. Error 0x%08x.", hr);
		goto exit;
	}
	
	// Transfer ownership on success.
	//
	ret = obj;
	obj = nullptr;

exit:

	if (obj)
	{
		Py_DECREF(obj);
		obj = nullptr;
	}
	return ret;
}
