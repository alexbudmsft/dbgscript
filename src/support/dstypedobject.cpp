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
#include <strsafe.h>

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

	hr = hostCtxt->DebugSymbols->GetTypeName(
		typObj->TypedData.ModBase,
		typObj->TypedData.TypeId,
		STRING_AND_CCH(typObj->TypeName),
		nullptr);
	if (FAILED(hr))
	{
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: Failed to get type name. Error 0x%08x.\n", hr);
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
	_In_z_ const char* type,
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

	hr = StringCchCopyA(STRING_AND_CCH(typObj->TypeName), type);
	assert(SUCCEEDED(hr));

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
		hostCtxt->DebugControl->Output(
			DEBUG_OUTPUT_ERROR,
			"Error: No such field '%s'.\n", fieldName);
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
