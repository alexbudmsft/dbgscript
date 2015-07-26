#include "typedobject.h"
#include <strsafe.h>
#include <structmember.h>

struct TypedObject
{
	PyObject_HEAD

	// Size of the symbol, in bytes. E.g. int => 4.
	//
	ULONG Size;

	// Name of the symbol. (T_STRING_INPLACE)
	//
	char Name[256];

	// Type of the symbol.
	//
	char TypeName[256];

	UINT64 ModuleBase;
	ULONG TypeId;
	UINT64 VirtualAddress;
};

static PyMemberDef TypedObject_MemberDef[] =
{
	{ "size", T_ULONG, offsetof(TypedObject, Size), READONLY },
	{ "name", T_STRING_INPLACE, offsetof(TypedObject, Name), READONLY },
	{ "type", T_STRING_INPLACE, offsetof(TypedObject, TypeName), READONLY },
	{ "address", T_ULONGLONG, offsetof(TypedObject, VirtualAddress), READONLY },
	{ NULL }
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
	TypedObjectType.tp_doc = PyDoc_STR("dbgscript.Symbol objects");
	TypedObjectType.tp_members = TypedObject_MemberDef;
	TypedObjectType.tp_new = PyType_GenericNew;

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
	_In_ UINT64 virtualAddress)
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
	TypedObject* sym = (TypedObject*)obj;
	sym->Size = size;
	sym->ModuleBase = moduleBase;
	sym->TypeId = typeId;
	sym->VirtualAddress = virtualAddress;

	HRESULT hr = StringCchCopyA(STRING_AND_CCH(sym->Name), name);
	assert(SUCCEEDED(hr));

	hr = StringCchCopyA(STRING_AND_CCH(sym->TypeName), type);
	assert(SUCCEEDED(hr));

	return obj;
}
