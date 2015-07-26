#include "symbol.h"
#include <strsafe.h>
#include <structmember.h>

struct SymbolObj
{
	PyObject_HEAD

	// Size of the symbol, in bytes. E.g. int => 4.
	//
	ULONG Size;

	// Name of the symbol. (T_STRING_INPLACE)
	//
	char Name[256];
};

static PyMemberDef Symbol_MemberDef[] =
{
	{ "size", T_ULONG, offsetof(SymbolObj, Size), READONLY },
	{ "name", T_STRING_INPLACE, offsetof(SymbolObj, Name), READONLY },
	{ NULL }
};

static PyTypeObject SymbolType =
{
	PyVarObject_HEAD_INIT(0, 0)
	"dbgscript.Symbol",     /* tp_name */
	sizeof(SymbolObj)       /* tp_basicsize */
};

_Check_return_ bool
InitSymbolType()
{
	SymbolType.tp_flags = Py_TPFLAGS_DEFAULT;
	SymbolType.tp_doc = PyDoc_STR("dbgscript.Symbol objects");
	SymbolType.tp_members = Symbol_MemberDef;
	SymbolType.tp_new = PyType_GenericNew;

	// Finalize the type definition.
	//
	if (PyType_Ready(&SymbolType) < 0)
	{
		return false;
	}
	return true;
}

_Check_return_ PyObject*
AllocSymbolObj(
	_In_ ULONG size,
	_In_z_ const char* name)
{
	PyObject* obj = nullptr;

	// Alloc a single instance of the SymbolType class. (Calls __new__())
	// If the allocation fails, the allocator will set the appropriate exception
	// internally. (i.e. OOM)
	//
	obj = SymbolType.tp_new(&SymbolType, nullptr, nullptr);
	if (!obj)
	{
		return nullptr;
	}

	// Set up fields.
	//
	SymbolObj* sym = (SymbolObj*)obj;
	sym->Size = size;
	HRESULT hr = StringCchCopyA(STRING_AND_CCH(sym->Name), name);
	assert(SUCCEEDED(hr));

	return obj;
}