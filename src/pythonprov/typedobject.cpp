#include "typedobject.h"
#include <strsafe.h>
#include <structmember.h>
#include "process.h"
#include <wdbgexts.h>

struct ProcessObj;

struct TypedObject
{
	PyObject_HEAD

	// Name of the typObjbol. (T_STRING_INPLACE)
	//
	char Name[256];

	// Type of the typObjbol.
	//
	char TypeName[256];

	// Backpointer to process object.
	//
	ProcessObj* Process;

	// DbgEng typed-data information used for walking object hierarchies.
	//
	DEBUG_TYPED_DATA TypedData;

	// Is 'TypedData' valid?
	//
	bool TypedDataValid;
};

static PyMemberDef TypedObject_MemberDef[] =
{
	{ "size", T_ULONG, offsetof(TypedObject, TypedData.Size), READONLY },
	{ "name", T_STRING_INPLACE, offsetof(TypedObject, Name), READONLY },
	{ "type", T_STRING_INPLACE, offsetof(TypedObject, TypeName), READONLY },
	{ "address", T_ULONGLONG, offsetof(TypedObject, TypedData.Offset), READONLY },
	{ NULL }
};

static PyTypeObject TypedObjectType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.TypedObject",     /* tp_name */
	sizeof(TypedObject)       /* tp_basicsize */
};

static _Check_return_ PyObject*
allocSubTypedObject(
	_In_z_ const char* fieldName,
	_In_ const DEBUG_TYPED_DATA* typedData,
	_In_ ProcessObj* proc)
{
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

	// TODO: Get name and type name.
	//
	HRESULT hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), fieldName);
	assert(SUCCEEDED(hr));

	typObj->TypedData = *typedData;
	typObj->TypedDataValid = true;

	hr = GetDllGlobals()->DebugSymbols->GetTypeName(
		typObj->TypedData.ModBase,
		typObj->TypedData.TypeId,
		STRING_AND_CCH(typObj->TypeName),
		nullptr);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to get type name. Error 0x%08x.", hr);
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

// Get a field from a typed object, constructing a new TypedObject in the process.
//
static PyObject*
TypedObject_subscript(
	_In_ PyObject* self,
	_In_ PyObject* key)
{
	TypedObject* typedObj = (TypedObject*)self;
	PyObject* newTypedObj = nullptr;
	EXT_TYPED_DATA* request = nullptr;
	EXT_TYPED_DATA* responseBuf = nullptr;
	BYTE* requestBuf = nullptr;

	HRESULT hr = S_OK;
	if (!PyUnicode_Check(key))
	{
		PyErr_SetString(PyExc_TypeError, "key must be a unicode object");
		return nullptr;
	}

	if (!typedObj->TypedDataValid)
	{
		// This object has no typed data. It must have been a null ptr.
		//
		PyErr_SetString(PyExc_AttributeError, "Object has no typed data. Can't get fields.");
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
	request->InData = typedObj->TypedData;
	request->InStrIndex = sizeof(EXT_TYPED_DATA);

	// Must be NULL terminated.
	//
	memcpy(requestBuf + sizeof(EXT_TYPED_DATA), fieldName, fieldNameLen + 1);

	hr = GetDllGlobals()->DebugAdvanced->Request(
		DEBUG_REQUEST_EXT_TYPED_DATA_ANSI,
		request,
		reqSize,
		responseBuf,
		reqSize,
		nullptr);
	if (FAILED(hr))
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
	TypedObject_subscript,  // mp_subscript
	nullptr   // mp_ass_subscript
};

_Check_return_ bool
InitTypedObjectType()
{
	TypedObjectType.tp_flags = Py_TPFLAGS_DEFAULT;
	TypedObjectType.tp_doc = PyDoc_STR("dbgscript.typObjbol objects");
	TypedObjectType.tp_members = TypedObject_MemberDef;
	TypedObjectType.tp_new = PyType_GenericNew;
	TypedObjectType.tp_dealloc = TypedObject_dealloc;
	TypedObjectType.tp_as_mapping = &TypedObject_MappingDef;

	// Finalize the type definition.
	//
	if (PyType_Ready(&TypedObjectType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocTypedObject(
	_In_ ULONG size,
	_In_z_ const char* name,
	_In_z_ const char* type,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_In_ ProcessObj* proc)
{
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

	HRESULT hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), name);
	assert(SUCCEEDED(hr));

	hr = StringCchCopyA(STRING_AND_CCH(typObj->TypeName), type);
	assert(SUCCEEDED(hr));

	// Can't generate typed data for null pointers. Then again, doesn't matter
	// much since can't traverse a null pointer anyway.
	//
	if (virtualAddress)
	{
		// TODO: Encapsulate this logic into a separate class perhaps.
		//
		EXT_TYPED_DATA request = {};
		EXT_TYPED_DATA response = {};
		request.Operation = EXT_TDOP_SET_FROM_TYPE_ID_AND_U64;
		request.InData.ModBase = moduleBase;
		request.InData.Offset = virtualAddress;
		request.InData.TypeId = typeId;

		hr = GetDllGlobals()->DebugAdvanced->Request(
			DEBUG_REQUEST_EXT_TYPED_DATA_ANSI,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			nullptr);
		if (FAILED(hr))
		{
			PyErr_Format(PyExc_OSError, "EXT_TDOP_SET_FROM_TYPE_ID_AND_U64 operation failed. Error 0x%08x.", hr);
			goto exit;
		}

		typObj->TypedData = response.OutData;
		typObj->TypedDataValid = true;

		assert(typObj->TypedData.Offset == virtualAddress);
	}
	else
	{
		typObj->TypedData.Size = size;
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
