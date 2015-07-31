#include <windows.h>

#pragma warning(push)
#pragma warning(disable: 4091)
#define _NO_CVCONST_H
#include <dbghelp.h>
#pragma warning(pop)

#include "typedobject.h"
#include <strsafe.h>
#include <structmember.h>
#include "process.h"
#include <wdbgexts.h>
#include "util.h"
#include "../util.h"

struct ProcessObj;

// From typedata.hpp.
//
#define DBG_NATIVE_TYPE_BASE    0x80000000
#define DBG_GENERATED_TYPE_BASE 0x80001000

enum BuiltinType
{
	DNTYPE_VOID = DBG_NATIVE_TYPE_BASE,
	DNTYPE_CHAR,
	DNTYPE_WCHAR_T,
	DNTYPE_INT8,
	DNTYPE_INT16,
	DNTYPE_INT32,
	DNTYPE_INT64,
	DNTYPE_UINT8,
	DNTYPE_UINT16,
	DNTYPE_UINT32,
	DNTYPE_UINT64,
	DNTYPE_FLOAT32,
	DNTYPE_FLOAT64,
	DNTYPE_FLOAT80,
	DNTYPE_BOOL,
	DNTYPE_LONG32,
	DNTYPE_ULONG32,
	DNTYPE_HRESULT,

	//
	// The following types aren't true native types but
	// are very basic aliases for native types that
	// need special identification.  For example, WCHAR
	// is here so that the debugger knows it's characters
	// and not just an unsigned short.
	//

	DNTYPE_WCHAR,

	//
	// Artificial type to mark cases where type information
	// is coming from the contained CLR value.
	//

	DNTYPE_CLR_TYPE,

	//
	// Artificial function type for CLR methods.
	//

	DNTYPE_MSIL_METHOD,
	DNTYPE_CLR_METHOD,
	DNTYPE_CLR_INTERNAL,

	//
	// Artificial pointer types for special-case handling
	// of things like vtables.
	//

	DNTYPE_PTR_FUNCTION32,
	DNTYPE_PTR_FUNCTION64,

	//
	// Placeholder for objects that don't have valid
	// type information but still need to be represented
	// for other reasons, such as enumeration.
	//

	DNTYPE_NO_TYPE,
	DNTYPE_ERROR,

	//  Types used by the Data Model for displaying children data
	DNTYPE_RAW_VIEW,
	DNTYPE_CONTINUATION,

	DNTYPE_END_MARKER
};

struct TypedObjectValue
{
	// See TypedObject.TypedData.BaseTypeId for type.
	//

	union
	{
		BYTE ByteVal;
		WORD WordVal;
		DWORD DwVal;
		INT64 I64Val;
		UINT64 UI64Val;
		BOOL BoolVal;
		float FloatVal;
		double DoubleVal;
	} Value;
};

struct TypedObject
{
	PyObject_HEAD

	// Name of the typObjbol. (T_STRING_INPLACE).
	//
	char Name[MAX_SYMBOL_NAME_LEN];

	// Type of the typObjbol.
	//
	char TypeName[MAX_SYMBOL_NAME_LEN];

	// Backpointer to process object.
	//
	ProcessObj* Process;

	// DbgEng typed-data information used for walking object hierarchies.
	//
	DEBUG_TYPED_DATA TypedData;

	// Is 'TypedData' valid?
	//
	bool TypedDataValid;

	// Value if this object represents a primitive type.
	// I.e. TypedData.Tag == SymTagBaseType.
	//
	TypedObjectValue Value;

	// Has the value been initialized?
	//
	bool ValueValid;
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

// Call when you already have a DEBUG_TYPED_DATA you want wrapped in a TypedObject.
//
static _Check_return_ PyObject*
allocSubTypedObject(
	_In_z_ const char* name,
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

	HRESULT hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), name);
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

static bool checkTypedData(
	_In_ TypedObject* typedObj)
{
	if (!typedObj->TypedDataValid)
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
	CHECK_ABORT;
	TypedObject* typObj = (TypedObject*)self;
	PyObject* ret = nullptr;

	if (!checkTypedData(typObj))
	{
		return nullptr;
	}

	if (typObj->TypedData.Tag != SymTagPointerType &&
		typObj->TypedData.Tag != SymTagArrayType)
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
	request.InData = typObj->TypedData;
	request.In64 = index;

	static_assert(sizeof(request) == sizeof(response),
		"Request and response must be equi-sized");

	HRESULT hr = GetDllGlobals()->DebugAdvanced->Request(
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
	CHECK_ABORT;
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
	assert(typObj->ValueValid);
	assert(typObj->TypedDataValid);
	const TypedObjectValue* cValue = &typObj->Value;
	DEBUG_TYPED_DATA* typedData = &typObj->TypedData;
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
				typObj->TypeName);
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
	CHECK_ABORT;

	// String formatting is really slow.
	//
	return PyUnicode_FromString("dbgscript.TypedObject");
}

static PyObject*
TypedObject_get_value(
	_In_ PyObject* self,
	_In_opt_ void* /* closure */)
{
	CHECK_ABORT;
	TypedObject* typObj = (TypedObject*)self;
	PyObject* ret = nullptr;

	if (!typObj->TypedDataValid)
	{
		PyErr_SetString(PyExc_AttributeError, "No typed data available.");
		return nullptr;
	}

	bool primitiveType = false;

	switch (typObj->TypedData.Tag)
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
	assert(typObj->TypedData.Size <= 8);
	HRESULT hr = GetDllGlobals()->DebugSymbols->ReadTypedDataVirtual(
		typObj->TypedData.Offset,
		typObj->TypedData.ModBase,
		typObj->TypedData.TypeId,
		&typObj->Value.Value,
		sizeof(typObj->Value.Value),
		&cbRead);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "Failed to read typed data. Error 0x%08x.", hr);
		goto exit;
	}
	assert(cbRead == typObj->TypedData.Size);
	
	// Value has been populated.
	//
	typObj->ValueValid = true;

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

	if (typObj->TypedData.Tag != SymTagPointerType &&
		typObj->TypedData.Tag != SymTagArrayType)
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

	assert(tmp->TypedDataValid);
	const ULONG elemSize = tmp->TypedData.Size;

	// Release the temporary object.
	//
	Py_DECREF(tmp);

	return typObj->TypedData.Size / elemSize;
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

	if (name)
	{
		hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), name);
		assert(SUCCEEDED(hr));
	}
	else
	{
		hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), "<unnamed>");
		assert(SUCCEEDED(hr));
	}

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
