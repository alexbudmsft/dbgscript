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
			"Error: Failed to get type name. Error 0x%08x.", hr);
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
		typObj->TypedData.Size = size;
	}

exit:
	return hr;
}

