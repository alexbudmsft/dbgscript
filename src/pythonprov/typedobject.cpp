#include <windows.h>

#include "typedobject.h"
#include <strsafe.h>
#include <structmember.h>
#include "process.h"
#include "util.h"
#include "common.h"

struct TypedObject
{
	PyObject_HEAD

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
	_In_ const DEBUG_TYPED_DATA* typedData)
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

static bool
checkTypedData(
	_In_ TypedObject* typedObj)
{
	if (!typedObj->Data.TypedDataValid)
	{
		// This object has no typed data. It must have been a null ptr.
		//
		PyErr_SetString(PyExc_ValueError, "Object has no typed data. Can't get fields.");
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
	HRESULT hr = S_OK;
	CHECK_ABORT(hostCtxt);

	TypedObject* typObj = (TypedObject*)self;
	PyObject* ret = nullptr;

	if (!checkTypedData(typObj))
	{
		return nullptr;
	}

	DEBUG_TYPED_DATA typedData = {0};
	hr = DsTypedObjectGetArrayElement(
		hostCtxt,
		&typObj->Data,
		index,
		&typedData);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
		goto exit;
	}

	// Array elements have no name.
	//
	ret = allocSubTypedObject(ARRAY_ELEM_NAME, &typedData);
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

	DEBUG_TYPED_DATA typedData = {0};
	hr = DsTypedObjectGetField(
		hostCtxt,
		&typedObj->Data,
		fieldName,
		&typedData);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsTypedObjectGetField failed. Error 0x%08x.", hr);
		goto exit;
	}

	newTypedObj = allocSubTypedObject(fieldName, &typedData);
	if (!newTypedObj)
	{
		goto exit;
	}

exit:

	return newTypedObj;
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
			assert(typedData->Size == sizeof(bool));
			ret = PyBool_FromLong(cValue->Value.BoolVal);
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
			PyErr_Format(PyExc_ValueError, "Unsupported type id: %d (%s)",
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

//------------------------------------------------------------------------------
// Function: TypedObject_getattro
//
// Description:
//
//  Virtual attribute getter for Typed Objects. This will work as 
//
// Parameters:
//
//  self - pointer to Lua state.
//  attr - pointer to Lua state.
//
// Returns:
//
//  One result: The name of the TypedObject.
//
// Notes:
//
static PyObject*
TypedObject_getattro(
	_In_ PyObject* self,
	_In_ PyObject* attr)
{
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;

	// First, call the generic attribute handler, to handle methods/members/getset.
	// I.e. the usual Python stuff.
	//
	PyObject* ret = PyObject_GenericGetAttr(self, attr);
	if (ret)
	{
		// Success: No special handling, just pass through.
		//
		return ret;
	}

	assert(PyErr_Occurred());

	// If it was something other than an attribute error, pass it through.
	// We don't want to swallow other error types.
	//
	if (!PyErr_ExceptionMatches(PyExc_AttributeError))
	{
		assert(!ret);
		return nullptr;
	}
	
	// We got an attribute error: This means the attribute was not present.
	// => attempt a struct field lookup.
	//
	
	// Clear the attribute error.
	//
	PyErr_Clear();

	// Get the key as an ANSI string.
	//
	PyObject* asciiStrAsBytes = PyUnicode_AsASCIIString(attr);
	if (!asciiStrAsBytes)
	{
		return nullptr;
	}

	const char* fieldName = PyBytes_AsString(asciiStrAsBytes);
	if (!fieldName)
	{
		return nullptr;
	}

	TypedObject* typedObj = (TypedObject*)self;
	DEBUG_TYPED_DATA typedData = {0};
	HRESULT hr = DsTypedObjectGetField(
		hostCtxt,
		&typedObj->Data,
		fieldName,
		&typedData);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsTypedObjectGetField failed. Error 0x%08x.", hr);
		return nullptr;
	}

	return allocSubTypedObject(fieldName, &typedData);
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

	if (!checkTypedData(typObj))
	{
		return nullptr;
	}

	if (!DsTypedObjectIsPrimitive(&typObj->Data))
	{
		PyErr_SetString(PyExc_ValueError, "Not a primitive type.");
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
	DbgScriptHostContext* hostCtxt = GetPythonProvGlobals()->HostCtxt;
	
	TypedObject* typObj = (TypedObject*)self;

	if (!checkTypedData(typObj))
	{
		return -1;
	}

	if (typObj->Data.TypedData.Tag != SymTagArrayType)
	{
		// Not array.
		//
		PyErr_SetString(PyExc_ValueError, "Object not array.");
		return -1;
	}

	// Get the zero'th item in order to get its size.
	//
	DEBUG_TYPED_DATA typedData = {0};
	HRESULT hr = DsTypedObjectGetArrayElement(
		hostCtxt,
		&typObj->Data,
		0,
		&typedData);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_OSError, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
		return -1;
	}

	const ULONG elemSize = typedData.Size;

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
	TypedObjectType.tp_doc = PyDoc_STR("dbgscript.TypedObject objects");
	TypedObjectType.tp_members = TypedObject_MemberDef;
	TypedObjectType.tp_getset = TypedObject_GetSetDef;
	TypedObjectType.tp_new = PyType_GenericNew;
	TypedObjectType.tp_str = TypedObject_str;
	TypedObjectType.tp_as_mapping = &TypedObject_MappingDef;
	TypedObjectType.tp_as_sequence = &s_SequenceMethodsDef;
	TypedObjectType.tp_getattro = TypedObject_getattro;

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
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress)
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

	hr = DsInitializeTypedObject(
		GetPythonProvGlobals()->HostCtxt,
		size,
		name,
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
