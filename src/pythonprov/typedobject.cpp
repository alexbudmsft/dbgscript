#include "typedobject.h"
#include <strsafe.h>
#include <structmember.h>
#include "process.h"

struct ProcessObj;

struct TypedObject
{
	PyObject_HEAD

	// Size of the typObjbol, in bytes. E.g. int => 4.
	//
	ULONG Size;

	// Name of the typObjbol. (T_STRING_INPLACE)
	//
	char Name[256];

	// Type of the typObjbol.
	//
	char TypeName[256];

	UINT64 ModuleBase;
	ULONG TypeId;
	UINT64 VirtualAddress;

	// Backpointer to process object.
	//
	ProcessObj* Parent;
};

static PyMemberDef TypedObject_MemberDef[] =
{
	{ "size", T_ULONG, offsetof(TypedObject, Size), READONLY },
	{ "name", T_STRING_INPLACE, offsetof(TypedObject, Name), READONLY },
	{ "type", T_STRING_INPLACE, offsetof(TypedObject, TypeName), READONLY },
	{ "address", T_ULONGLONG, offsetof(TypedObject, VirtualAddress), READONLY },
	{ NULL }
};

static PyObject*
TypedObject_subscript(
	_In_ PyObject* self,
	_In_ PyObject* key)
{
	TypedObject* typedObj = (TypedObject*)self;
	if (!PyUnicode_Check(key))
	{
		PyErr_SetString(PyExc_TypeError, "key must be a unicode object");
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

	// Lookup module name.
	const char* modName = ProcessObjGetModuleName(
		typedObj->Parent, typedObj->ModuleBase);
	modName;
	return nullptr;
}

static void
TypedObject_dealloc(
	_In_ PyObject* self)
{
	TypedObject* typObj = (TypedObject*)self;

	// Release ref on the parent process object.
	//
	Py_DECREF(typObj->Parent);

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

static PyTypeObject TypedObjectType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.TypedObject",     /* tp_name */
	sizeof(TypedObject)       /* tp_basicsize */
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
	typObj->Size = size;
	typObj->ModuleBase = moduleBase;
	typObj->TypeId = typeId;
	typObj->VirtualAddress = virtualAddress;

	Py_INCREF(proc);
	typObj->Parent = proc;

	HRESULT hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), name);
	assert(SUCCEEDED(hr));

	hr = StringCchCopyA(STRING_AND_CCH(typObj->TypeName), type);
	assert(SUCCEEDED(hr));

	return obj;
}
