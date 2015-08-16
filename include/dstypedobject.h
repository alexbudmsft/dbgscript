//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dstypedobject.h
// @Author: alexbud
//
// Purpose:
//
//  DbgScript Typed Object.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************
#pragma once

#pragma warning(push)
#pragma warning(disable: 4091)
#define _NO_CVCONST_H
#include <dbghelp.h>
#pragma warning(pop)

#include <wdbgexts.h>

// Limits of our extension.
//
const int MAX_MODULE_NAME_LEN = 256;
const int MAX_SYMBOL_NAME_LEN = 512;

// Name to use for anonymous array elements.
//
#define ARRAY_ELEM_NAME "<arr-elem>"

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

// TypedObjectValue - union of all base primitive types supported.
//
struct TypedObjectValue
{
	// See DbgScriptTypedObject.TypedData.BaseTypeId for type.
	//

	union
	{
		BYTE ByteVal;
		WORD WordVal;
		DWORD DwVal;
		INT64 I64Val;
		UINT64 UI64Val;
		bool BoolVal;
		float FloatVal;
		double DoubleVal;
	} Value;
};

// DbgScriptTypedObject - Object that models a typed object in the target's
// virtual address space.
//
struct DbgScriptTypedObject
{
	// Name of the typObjbol.
	//
	char Name[MAX_SYMBOL_NAME_LEN];

	// Type of the typObjbol.
	//
	char TypeName[MAX_SYMBOL_NAME_LEN];

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

_Check_return_ HRESULT
DsInitializeTypedObject(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_z_ const char* type,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_Out_ DbgScriptTypedObject* typObj);

_Check_return_ HRESULT
DsWrapTypedData(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* name,
	_In_ const DEBUG_TYPED_DATA* typedData,
	_Out_ DbgScriptTypedObject* typObj);

_Check_return_ HRESULT
DsTypedObjectGetField(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptTypedObject* typedObj,
	_In_z_ const char* fieldName,
	_Out_ DEBUG_TYPED_DATA* outData);

_Check_return_ HRESULT
DsTypedObjectGetArrayElement(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptTypedObject* typedObj,
	_In_ UINT64 index,
	_Out_ DEBUG_TYPED_DATA* outData);

_Check_return_ bool
DsTypedObjectIsPrimitive(
	_In_ DbgScriptTypedObject* typObj);

