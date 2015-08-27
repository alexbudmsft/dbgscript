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

static bool
checkTypedData(
	_In_ DbgScriptTypedObject* typedObj,
	_In_ bool fRaise)
{
	if (!typedObj->TypedDataValid)
	{
		if (fRaise)
		{
			// This object has no typed data. It must have been a null or bad ptr.
			//
			rb_raise(rb_eArgError, "Object has no typed data.");
		}
		else
		{
			return false;
		}
	}
	return true;
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

//------------------------------------------------------------------------------
// Function: TypedObject_get_runtime_obj
//
// Synopsis:
// 
//  obj.get_runtime_obj -> TypedObject
//
// Description:
//
//  Discovers the runtime type of an object or pointer-to an object by checking
//  its vtable. Creates a new object of this type. Throws if 'obj' does not 
//  have a vtable.
//  
static VALUE
TypedObject_get_runtime_obj(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	HRESULT hr = S_OK;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;
	DbgScriptTypedObject* newTypObj = nullptr;
	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	// Alloc a new typed object.
	//
	VALUE newObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->TypedObjectClass);

	Data_Get_Struct(newObj, DbgScriptTypedObject, newTypObj);
	
	hr = DsTypedObjectGetRuntimeType(hostCtxt, typObj, newTypObj);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "DsTypedObjectGetRuntimeType failed. Error: 0x%08x", hr);
	}
	
	return newObj;
}

//------------------------------------------------------------------------------
// Function: TypedObject_read_string
//
// Synopsis:
//
//  obj.read_string([count]) -> String
//
// Description:
//
//  Read an, optionally counted, ANSI string from the target process.
//
//  count is the (int) number of characters to read. -1 means read up to the
//  first NUL.
//
static VALUE
TypedObject_read_string(
	_In_ int argc,
	_In_reads_(argc) VALUE* argv,
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	HRESULT hr = S_OK;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;
	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	char buf[MAX_READ_STRING_LEN];
	UINT64 addr = 0;
	ULONG cbActualLen = 0;

	// Initialize to default value. -1 means 
	//
	int count = -1;

	checkTypedData(typObj, true /* fRaise */);

	addr = typObj->TypedData.Offset;

	if (argc > 1)
	{
		rb_raise(rb_eArgError, "wrong number of arguments");
	}
	else if (argc == 1)
	{
		count = NUM2INT(argv[0]);
	}

	if (!count || count > MAX_READ_STRING_LEN - 1)
	{
		rb_raise(rb_eArgError, "count supports at most %d and can't be 0", MAX_READ_STRING_LEN - 1);
	}
	
	hr = UtilReadAnsiString(hostCtxt, addr, STRING_AND_CCH(buf), count, &cbActualLen);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilReadAnsiString failed. Error 0x%08x.", hr);
	}

	return rb_str_new(buf, cbActualLen - 1);
}

//------------------------------------------------------------------------------
// Function: TypedObject_read_wide_string
//
// Synopsis:
//
//  obj.read_wide_string([count]) -> String
//
// Description:
//
//  Read an, optionally counted, wide string from the target process.
//
//  count is the (int) number of characters to read. -1 means read up to the
//  first NUL.
//
static VALUE
TypedObject_read_wide_string(
	_In_ int argc,
	_In_reads_(argc) VALUE* argv,
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	HRESULT hr = S_OK;
	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;
	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	WCHAR buf[MAX_READ_STRING_LEN];
	char utf8buf[MAX_READ_STRING_LEN * sizeof(WCHAR)];
	UINT64 addr = 0;
	ULONG cchActualLen = 0;

	// Initialize to default value. -1 means 
	//
	int count = -1;

	checkTypedData(typObj, true /* fRaise */);

	addr = typObj->TypedData.Offset;

	if (argc > 1)
	{
		rb_raise(rb_eArgError, "wrong number of arguments");
	}
	else if (argc == 1)
	{
		count = NUM2INT(argv[0]);
	}

	if (!count || count > MAX_READ_STRING_LEN - 1)
	{
		rb_raise(rb_eArgError, "count supports at most %d and can't be 0", MAX_READ_STRING_LEN - 1);
	}
	
	hr = UtilReadWideString(hostCtxt, addr, STRING_AND_CCH(buf), count, &cchActualLen);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "UtilReadWideString failed. Error 0x%08x.", hr);
	}

	// Convert to UTF8. Output will *not* be NUL-terminated because input wasn't.
	//
	const int cbWritten = WideCharToMultiByte(
		CP_UTF8, 0, buf, cchActualLen - 1, utf8buf, sizeof utf8buf, nullptr, nullptr);
	if (!cbWritten)
	{
		DWORD err = GetLastError();
		rb_raise(rb_eRuntimeError, "WideCharToMultiByte failed. Error %d.", err);
	}

	return rb_utf8_str_new(utf8buf, cbWritten);
}

//------------------------------------------------------------------------------
// Function: TypedObject_value
//
// Synopsis:
//
//  obj.value -> varies
//
// Description:
//
//  Return the value of a primitive type. Throws if object is not of a primitive
//  type.
//
static VALUE
TypedObject_value(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);
	
	checkTypedData(typObj, true /* fRaise */);

	if (!DsTypedObjectIsPrimitive(typObj))
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
		rb_raise(rb_eRuntimeError, "Failed to read typed data. Error 0x%08x.", hr);
	}
	assert(cbRead == typObj->TypedData.Size);
	
	// Value has been populated.
	//
	typObj->ValueValid = true;

	return rbValueFromCValue(typObj);
}

//------------------------------------------------------------------------------
// Function: TypedObject_data_size
//
// Synopsis:
// 
//  obj.data_size -> Integer
//
// Description:
//
//  Return the size of the object in bytes.
//
// Notes:
//
//  Don't want to call it 'size' because 'size' is a built-in method on arrays
//  in Ruby which is an alias for 'length'.
//
static VALUE
TypedObject_data_size(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	return ULONG2NUM(typObj->TypedData.Size);
}

//------------------------------------------------------------------------------
// Function: TypedObject_name
//
// Synopsis:
// 
//  obj.name -> String
//
// Description:
//
//  Return the name of the object.
//
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

//------------------------------------------------------------------------------
// Function: TypedObject_type
//
// Synopsis:
// 
//  obj.type -> String
//
// Description:
//
//  Return the type name of the object. This does not include the module prefix.
//
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

//------------------------------------------------------------------------------
// Function: TypedObject_module
//
// Synopsis:
// 
//  obj.module -> String
//
// Description:
//
//  Return the module name of the object's type.
//
static VALUE
TypedObject_module(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;

	CHECK_ABORT(hostCtxt);
	
	DbgScriptTypedObject* typObj = nullptr;

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);
	
	return rb_str_new2(typObj->ModuleName);
}

//------------------------------------------------------------------------------
// Function: TypedObject_address
//
// Synopsis:
// 
//  obj.address -> Integer
//
// Description:
//
//  Return the address of the object.
//
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
		rb_raise(rb_eRuntimeError, "DsWrapTypedData failed. Error 0x%08x.", hr);
	}

	return newObj;
}

//------------------------------------------------------------------------------
// Function: TypedObject_method_missing
//
// Description:
//
//  Called when Ruby can't find a method on this object. We use this implement
//  attempted field-lookup.
//
// Parameters:
//
//  self - Receiver.
//  methodId - ID of method that could not be found (symbol). We will consider
//   this to be the field name of interest.
//
// Returns:
//
//  One result: Typed Object.
//
// Notes:
//
static VALUE
TypedObject_method_missing(
	_In_ int argc,
	_In_reads_(argc) VALUE *argv,
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	DbgScriptTypedObject* typObj = nullptr;
	CHECK_ABORT(hostCtxt);

	VALUE methodStr = rb_sym2str(argv[0]);
	const char* fieldName = StringValuePtr(methodStr);

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);
	const ID methodMissing = rb_intern("method_missing");

	bool fOk = checkTypedData(typObj, false);
	if (!fOk)
	{
		// Call super class.
		//
		VALUE super = rb_class_superclass(CLASS_OF(self));
		return rb_funcallv(super, methodMissing, argc, argv);
	}
	
	DEBUG_TYPED_DATA typedData = {0};
	HRESULT hr = DsTypedObjectGetField(
		hostCtxt,
		typObj,
		fieldName,
		false,
		&typedData);
	if (FAILED(hr))
	{
		// Call super class.
		//
		VALUE super = rb_class_superclass(CLASS_OF(self));
		return rb_funcallv(super, methodMissing, argc, argv);
	}
	
	return allocTypedObjFromTypedData(fieldName, &typedData);
}

//------------------------------------------------------------------------------
// Function: TypedObject_length
//
// Synopsis:
// 
//  obj.length -> Integer
//
// Description:
//
//  Return the length of an array. Throws if object is not an array.
//
static VALUE
TypedObject_length(
	_In_ VALUE self)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	DbgScriptTypedObject* typObj = nullptr;
	CHECK_ABORT(hostCtxt);

	Data_Get_Struct(self, DbgScriptTypedObject, typObj);

	checkTypedData(typObj, true);
	
	if (typObj->TypedData.Tag != SymTagArrayType)
	{
		// Not array.
		//
		rb_raise(rb_eArgError, "Object not array.");
	}

	// Get the zero'th item in order to get its size.
	//
	DEBUG_TYPED_DATA typedData = {0};
	HRESULT hr = DsTypedObjectGetArrayElement(
		hostCtxt,
		typObj,
		0 /* index */,
		&typedData);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
	}

	const ULONG elemSize = typedData.Size;

	return ULONG2NUM(typObj->TypedData.Size / elemSize);
}

_Check_return_ VALUE
AllocTypedObject(
	_In_ ULONG size,
	_In_opt_z_ const char* name,
	_In_ ULONG typeId,
	_In_ UINT64 moduleBase,
	_In_ UINT64 virtualAddress)
{
	DbgScriptHostContext* hostCtxt = GetRubyProvGlobals()->HostCtxt;
	
	// Allocate a Typed Object.
	//
	VALUE typObj = rb_class_new_instance(
		0, nullptr, GetRubyProvGlobals()->TypedObjectClass);

	DbgScriptTypedObject* obj = nullptr;

	Data_Get_Struct(typObj, DbgScriptTypedObject, obj);

	HRESULT hr = DsInitializeTypedObject(
		hostCtxt,
		size,
		name,
		typeId,
		moduleBase,
		virtualAddress,
		obj);
	if (FAILED(hr))
	{
		rb_raise(rb_eRuntimeError, "DsInitializeTypedObject failed. Error 0x%08x.", hr);
	}

	return typObj;
}

//------------------------------------------------------------------------------
// Function: TypedObject_get_item
//
// Synopsis:
// 
//  obj[key] -> TypedObject
//
// Description:
//
//  Dictionary-style lookup on the object. If 'key' is a String attempts field
//  lookup. Otherwise if convertible to Integer attempts array access.
//
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
	
	checkTypedData(typObj, true);
	
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
			true,
			&typedData);
		if (FAILED(hr))
		{
			rb_raise(rb_eRuntimeError, "DsTypedObjectGetField failed. Error 0x%08x.", hr);
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
			rb_raise(rb_eRuntimeError, "DsTypedObjectGetArrayElement failed. Error 0x%08x.", hr);
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
		"data_size",
		RUBY_METHOD_FUNC(TypedObject_data_size),
		0 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"type",
		RUBY_METHOD_FUNC(TypedObject_type),
		0 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"module",
		RUBY_METHOD_FUNC(TypedObject_module),
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
	
	rb_define_method(
		typedObjectClass,
		"get_runtime_obj",
		RUBY_METHOD_FUNC(TypedObject_get_runtime_obj),
		0 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"read_string",
		RUBY_METHOD_FUNC(TypedObject_read_string),
		-1 /* argc */);
	
	rb_define_method(
		typedObjectClass,
		"read_wide_string",
		RUBY_METHOD_FUNC(TypedObject_read_wide_string),
		-1 /* argc */);
	
	// Length, if object is an array.
	//
	rb_define_method(
		typedObjectClass,
		"length",
		RUBY_METHOD_FUNC(TypedObject_length),
		0 /* argc */);
	
    rb_define_alias(typedObjectClass, "size", "length");
	
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

	// Implement 'method_missing' so that we can support virtual properties.
	//
	rb_define_method(
		typedObjectClass,
		"method_missing",
		RUBY_METHOD_FUNC(TypedObject_method_missing),
		-1 /* argc */);
	
	// Prevent scripter from instantiating directly.
	//
	LockDownClass(typedObjectClass);
	
	// Save the thread class so others can instantiate it.
	//
	GetRubyProvGlobals()->TypedObjectClass = typedObjectClass;
}
