//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: typedobject.cpp
// @Author: alexbud
//
// Purpose:
//
//  TypedObject class for Ruby Provider.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  
#include "common.h"
#include "typedobject.h"

//------------------------------------------------------------------------------
// Function: TypedObject_free
//
// Description:
//
//  Frees a DbgScriptTypedObject object allocated by 'TypedObject_alloc'.
//  
// Returns:
//
// Notes:
//
static void
TypedObject_free(
	_In_ void* obj)
{
	DbgScriptTypedObject* o = (DbgScriptTypedObject*)obj;
	delete o;
}

//------------------------------------------------------------------------------
// Function: TypedObject_alloc
//
// Description:
//
//  Allocates a Ruby-wrapped DbgScriptTypedObject object.
//  
// Returns:
//
// Notes:
//
static VALUE
TypedObject_alloc(
	_In_ VALUE klass)
{
	DbgScriptTypedObject* obj = new DbgScriptTypedObject;
	memset(obj, 0, sizeof(*obj));

	return Data_Wrap_Struct(klass, nullptr /* mark */, TypedObject_free, obj);
}

//------------------------------------------------------------------------------
// Function: rbValueFromCValue
//
// Description:
//
//  Convert a C Value into a Ruby value.
//  
// Returns:
//
// Notes:
//
static VALUE
rbValueFromCValue(
	_In_ DbgScriptTypedObject* typObj)
{
	assert(typObj->ValueValid);
	assert(typObj->TypedDataValid);
	const TypedObjectValue* cValue = &typObj->Value;
	DEBUG_TYPED_DATA* typedData = &typObj->TypedData;
	VALUE ret = 0;

	if (typedData->Tag == SymTagPointerType)
	{
		ret = ULL2NUM(cValue->Value.UI64Val);
	}
	else if (typedData->Tag == SymTagEnum)
	{
		ret = ULONG2NUM(cValue->Value.DwVal);
	}
	else
	{
		assert(typedData->Tag == SymTagBaseType);

		switch (typedData->BaseTypeId)
		{
		case DNTYPE_CHAR:
		case DNTYPE_INT8:
		case DNTYPE_UINT8:
			assert(typedData->Size == 1);
			ret = INT2FIX(cValue->Value.ByteVal);
			break;
		case DNTYPE_INT16:
		case DNTYPE_UINT16:
		case DNTYPE_WCHAR:
			static_assert(sizeof(WCHAR) == sizeof(WORD), "Assume WCHAR is 2 bytes");
			assert(typedData->Size == sizeof(WORD));
			ret = INT2FIX(cValue->Value.WordVal);
			break;
		case DNTYPE_INT32:
		case DNTYPE_LONG32:
			assert(typedData->Size == sizeof(DWORD));
			ret = INT2FIX(cValue->Value.DwVal);
			break;
		case DNTYPE_UINT32:
		case DNTYPE_ULONG32:
			assert(typedData->Size == sizeof(DWORD));
			ret = ULONG2NUM(cValue->Value.DwVal);
			break;
		case DNTYPE_INT64:
			assert(typedData->Size == sizeof(INT64));
			ret = LL2NUM(cValue->Value.I64Val);
			break;
		case DNTYPE_UINT64:
			assert(typedData->Size == sizeof(UINT64));
			ret = ULL2NUM(cValue->Value.UI64Val);
			break;
		case DNTYPE_BOOL:
			assert(typedData->Size == sizeof(bool));
			ret = cValue->Value.BoolVal ? Qtrue : Qfalse;
			break;
		case DNTYPE_FLOAT32:
			assert(typedData->Size == sizeof(float));
			ret = DBL2NUM(cValue->Value.FloatVal);
			break;
		case DNTYPE_FLOAT64:
			assert(typedData->Size == sizeof(double));
			ret = DBL2NUM(cValue->Value.DoubleVal);
			break;
		default:
			rb_raise(rb_eRuntimeError, "Unsupported type id: %d (%s)",
				typedData->BaseTypeId,
				typObj->TypeName);
			break;
		}
	}
	return ret;
}

static VALUE
TypedObject_value(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);
	
	if (!typObj->TypedDataValid)
	{
		rb_raise(rb_eRuntimeError, "No typed data available.");
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
		rb_raise(rb_eArgError, "Not a primitive type.");
	}

	// Read the appropriate size from memory.
	//
	// What primitive type is bigger than 8 bytes?
	//
	ULONG cbRead = 0;
	assert(typObj->TypedData.Size <= 8);
	HRESULT hr = hostCtxt->DebugSymbols->ReadTypedDataVirtual(
		typObj->TypedData.Offset,
		typObj->TypedData.ModBase,
		typObj->TypedData.TypeId,
		&typObj->Value.Value,
		sizeof(typObj->Value.Value),
		&cbRead);
	if (FAILED(hr))
	{
		rb_raise(rb_eSystemCallError, "Failed to read typed data. Error 0x%08x.", hr);
	}
	assert(cbRead == typObj->TypedData.Size);
	
	// Value has been populated.
	//
	typObj->ValueValid = true;

	return rbValueFromCValue(typObj);
}

static VALUE
TypedObject_size(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	return ULONG2NUM(typObj->TypedData.Size);
}

static VALUE
TypedObject_name(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	return rb_str_new2(typObj->Name);
}

static VALUE
TypedObject_type(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);
	
	return rb_str_new2(typObj->TypeName);
}

static VALUE
TypedObject_address(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	return ULL2NUM(typObj->TypedData.Offset);
}

static void
checkTypedData(
	_In_ DbgScriptTypedObject* typedObj)
{
	if (!typedObj->TypedDataValid)
	{
		// This object has no typed data. It must have been a null ptr.
		//
		rb_raise(rb_eArgError, "Object has no typed data. Can't get fields.");
	}
}

static VALUE
allocTypedObjFromTypedData(
	_In_z_ const char* name,
	_In_ DEBUG_TYPED_DATA* typedData)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	VALUE newObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->TypedObjectClass);
	
	DbgScriptTypedObject* obj = nullptr;
	
	Data_Get_Struct(newObj, DbgScriptTypedObject, obj);
	
	HRESULT hr = DsWrapTypedData(hostCtxt, name, typedData, obj);
	if (FAILED(hr))
	{
		rb_raise(rb_eSystemCallError, "DsWrapTypedData failed. Error 0x%08x.", hr);
	}
	
	return newObj;
}

static VALUE
TypedObject_get_item(
	_In_ VALUE self,
	_In_ VALUE key)
{
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);
	
	checkTypedData(typObj);
	
	if (TYPE(key) == T_STRING)
	{
		// Field lookup.
		//
		const char* field = StringValuePtr(key);

		DEBUG_TYPED_DATA typedData = {0};
		hr = DsTypedObjectGetField(
			hostCtxt,
			typObj,
			field,
			&typedData);
		if (FAILED(hr))
		{
			rb_raise(rb_eSystemCallError, "DsTypedObjectGetField failed. Error 0x%08x.", hr);
		}
		return allocTypedObjFromTypedData(field, &typedData);
	}
	else
	{
		// Try convert to int.
		//
		VALUE intKey = rb_check_to_int(key);
		const int index = NUM2INT(intKey);
		
		// Array lookup.
		//
		DEBUG_TYPED_DATA typedData = {0};
		hr = DsTypedObjectGetArrayElement(
			hostCtxt,
			typObj,
			index,
			&typedData);
		if (FAILED(hr))
		{
			rb_raise(rb_eSystemCallError, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
		}
		return allocTypedObjFromTypedData(ARRAY_ELEM_NAME, &typedData);
	}
}

void
Init_TypedObject()
{
	VALUE typedObjectClass = rb_define_class_under(
		GetRubyProvGlobals()->DbgScriptModule,
		"TypedObject",
		rb_cObject);
	
	rb_define_alloc_func(typedObjectClass, TypedObject_alloc);

	rb_define_method(
		typedObjectClass,
		"value",
		RUBY_METHOD_FUNC(TypedObject_value),
		0 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"size",
		RUBY_METHOD_FUNC(TypedObject_size),
		0 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"type",
		RUBY_METHOD_FUNC(TypedObject_type),
		0 /* argc */);

	rb_define_method(
		typedObjectClass,
		"name",
		RUBY_METHOD_FUNC(TypedObject_name),
		0 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"address",
		RUBY_METHOD_FUNC(TypedObject_address),
		0 /* argc */);

	// Indexer method. Can take string or int key, for field or array access,
	// respectively.
	//
	// Does not support slicing, etc like regular arrays.
	//
    rb_define_method(
		typedObjectClass,
		"[]",
		RUBY_METHOD_FUNC(TypedObject_get_item),
		1);
	
	// Prevent scripter from instantiating directly.
	//
	LockDownClass(typedObjectClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->TypedObjectClass = typedObjectClass;
}