//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: dstypedobject.cpp
// @Author: alexbud
//
// Purpose:
//
//  Implements support routines for DbgScriptTypedObject.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "../common.h"
#include "util.h"
#include "symcache.h"
#include <strsafe.h>

//------------------------------------------------------------------------------
// Function: fillTypeAndModuleName
//
// Description:
//
//  Utility to fetch and populate the type and module names of a typed object.
//
// Parameters:
//
// Returns:
//
//  HRESULT.
//
// Notes:
//
static _Check_return_ HRESULT
fillTypeAndModuleName(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_Out_ DbgScriptTypedObject* typObj)
{
	HRESULT hr = S_OK;
	const char* cachedTypeName = nullptr;
	const char* cachedModName = nullptr;

	// Get type name.
	//
	const ModuleAndTypeId modAndTypeId = { typeId, moduleBase };
	cachedTypeName = GetCachedTypeName(
		hostCtxt,
		modAndTypeId);
	if (!cachedTypeName)
	{
		hr = E_FAIL;
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_TYPE_NAME);
		
		goto exit;
	}
	hr = StringCchCopyA(STRING_AND_CCH(typObj->TypeName), cachedTypeName);
	assert(SUCCEEDED(hr));
	
	// Get module name.
	//
	cachedModName = GetCachedModuleName(
		hostCtxt,
		moduleBase);
	if (!cachedModName)
	{
		hr = E_FAIL;
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_MODULE_NAME);
		
		goto exit;
	}

	hr = StringCchCopyA(STRING_AND_CCH(typObj->ModuleName), cachedModName);
	assert(SUCCEEDED(hr));
	
exit:
	return hr;
}

_Check_return_ HRESULT
DsWrapTypedData(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_z_ const char* name,
	_In_ const DEBUG_TYPED_DATA* typedData,
	_Out_ DbgScriptTypedObject* typObj)
{
	HRESULT hr = StringCchCopyA(STRING_AND_CCH(typObj->Name), name);
	assert(SUCCEEDED(hr));

	typObj->TypedData = *typedData;
	typObj->TypedDataValid = true;

	hr = fillTypeAndModuleName(
		hostCtxt,
		typObj->TypedData.TypeId,
		typObj->TypedData.ModBase,
		typObj);
	if (FAILED(hr))
	{
		goto exit;
	}
exit:
	return hr;
}

_Check_return_ HRESULT
DsInitializeTypedObject(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress,
	_Out_ DbgScriptTypedObject* typObj)
{
	HRESULT hr = S_OK;
	
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

	// Locals enumeration sometimes finds optimized-away locals. Those
	// don't have a real DEBUG_SYMBOL_ENTRY so we just pass a zero-filled one
	// through.
	//
	if (moduleBase)
	{
		hr = fillTypeAndModuleName(
			hostCtxt,
			typeId,
			moduleBase,
			typObj);
		if (FAILED(hr))
		{
			goto exit;
		}
	}
	else
	{
		hr = StringCchCopyA(STRING_AND_CCH(typObj->TypeName), "<invalid>");
		assert(SUCCEEDED(hr));
		hr = StringCchCopyA(STRING_AND_CCH(typObj->ModuleName), "<invalid>");
		assert(SUCCEEDED(hr));
	}

	// Can't generate typed data for null pointers. Then again, doesn't matter
	// much since can't traverse a null pointer anyway.
	//
	if (virtualAddress)
	{
		EXT_TYPED_DATA request = {};
		EXT_TYPED_DATA response = {};
		request.Operation = EXT_TDOP_SET_FROM_TYPE_ID_AND_U64;
		request.InData.ModBase = moduleBase;
		request.InData.Offset = virtualAddress;
		request.InData.TypeId = typeId;

		hr = hostCtxt->DebugAdvanced->Request(
			DEBUG_REQUEST_EXT_TYPED_DATA_ANSI,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			nullptr);
		if (FAILED(hr))
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: EXT_TDOP_SET_FROM_TYPE_ID_AND_U64 operation failed. Error 0x%08x.\n", hr);
			goto exit;
		}

		typObj->TypedData = response.OutData;
		typObj->TypedDataValid = true;

		assert(typObj->TypedData.Offset == virtualAddress);
	}
	else
	{
		typObj->TypedData.Size = size; // 0
		typObj->TypedData.Offset = virtualAddress; // 0
	}

exit:
	return hr;
}

_Check_return_ HRESULT
DsTypedObjectGetField(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptTypedObject* typedObj,
	_In_z_ const char* fieldName,
	_In_ bool fPrintMissing,
	_Out_ DEBUG_TYPED_DATA* outData)
{
	HRESULT hr = S_OK;
	EXT_TYPED_DATA* request = nullptr;
	EXT_TYPED_DATA* responseBuf = nullptr;
	BYTE* requestBuf = nullptr;
	const ULONG fieldNameLen = (ULONG)strlen(fieldName);
	const ULONG reqSize = sizeof(EXT_TYPED_DATA) + fieldNameLen + 1;

	requestBuf = (BYTE*)malloc(reqSize);
	if (!requestBuf)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	// Response buffer must be as big as request since dbgeng memcpy's from
	// request to response as a first step.
	//
	responseBuf = (EXT_TYPED_DATA*)malloc(reqSize);
	if (!responseBuf)
	{
		hr = E_OUTOFMEMORY;
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
		if (fPrintMissing)
		{
			hostCtxt->DebugControl->Output(
				DEBUG_OUTPUT_ERROR,
				"Error: No such field '%s'.\n", fieldName);
		}
		goto exit;
	}
	else if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: EXT_TDOP_GET_FIELD operation failed. Error 0x%08x.\n", hr);
		goto exit;
	}

	*outData = responseBuf->OutData;

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
	return hr;
}

_Check_return_ HRESULT
DsTypedObjectGetArrayElement(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ DbgScriptTypedObject* typObj,
	_In_ UINT64 index,
	_Out_ DEBUG_TYPED_DATA* outData)
{
	HRESULT hr = S_OK;
	EXT_TYPED_DATA request = {};
	EXT_TYPED_DATA response = {};

	if (typObj->TypedData.Tag != SymTagPointerType &&
		typObj->TypedData.Tag != SymTagArrayType)
	{
		// Not a pointer or array.
		//
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Object not a pointer or array.\n");
		hr = E_INVALIDARG;
		goto exit;
	}

	request.Operation = EXT_TDOP_GET_ARRAY_ELEMENT;
	request.InData = typObj->TypedData;
	request.In64 = index;

	static_assert(sizeof(request) == sizeof(response),
		"Request and response must be equi-sized");

	hr = hostCtxt->DebugAdvanced->Request(
		DEBUG_REQUEST_EXT_TYPED_DATA_ANSI,
		&request,
		sizeof(request),
		&response,
		sizeof(response),
		nullptr);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: EXT_TDOP_GET_ARRAY_ELEMENT operation failed. Error 0x%08x.\n", hr);
		goto exit;
	}

	*outData = response.OutData;
	
exit:
	return hr;
}

_Check_return_ bool
DsTypedObjectIsPrimitive(
	_In_ DbgScriptTypedObject* typObj)
{
	bool primitiveType = false;

	switch (typObj->TypedData.Tag)
	{
	case SymTagBaseType:
	case SymTagPointerType:
	case SymTagEnum:
		primitiveType = true;
		break;
	}
	
	return primitiveType;
}

//------------------------------------------------------------------------------
// Function: DsTypedObjectGetRuntimeType
//
// Description:
//
//  Get runtime type of an object by inspecting its vtable.
//
// Parameters:
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
DsTypedObjectGetRuntimeType(
	_In_ DbgScriptHostContext* hostCtxt,
	_In_ const DbgScriptTypedObject* typObj,
	_Out_ DbgScriptTypedObject* newTypObj)
{
	HRESULT hr = S_OK;
	UINT64 ptrVal = 0;
	char name[MAX_SYMBOL_NAME_LEN] = {};

	// Read the vptr.
	//
	UtilReadPointer(hostCtxt, typObj->TypedData.Offset, &ptrVal);

	hr = hostCtxt->DebugSymbols->GetNameByOffset(
		ptrVal, STRING_AND_CCH(name), nullptr, nullptr);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_NAME_BY_OFFSET,
			ptrVal,
			hr);
		goto exit;
	}

	// If the object has a vptr, it points to a vtable with a symbol like:
	//
	//   hkengine!HkLogImpl::`vftable'
	//
	char* found = strstr(name, "::`vftable'");
	if (!found)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_NO_VTABLE,
			ptrVal);
		goto exit;
	}

	// Null out the colon.
	//
	*found = 0;
	
	// Lookup typeid/moduleBase from type name.
	//
	ModuleAndTypeId* typeInfo = GetCachedSymbolType(hostCtxt, name);
	if (!typeInfo)
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			ERR_FAILED_GET_TYPE_ID,
			name,
			hr);
		goto exit;
	}

	// Initialize a new typed object based on this one.
	//
	hr = DsInitializeTypedObject(
		hostCtxt,
		typObj->TypedData.Size,
		typObj->Name,
		typeInfo->TypeId,
		typeInfo->ModuleBase,
		typObj->TypedData.Offset,
		newTypObj);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"DsInitializeTypedObject failed. Error 0x%08x.",
			hr);
		goto exit;
	}
	
exit:
	return hr;
}


